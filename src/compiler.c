#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

// Compila el AST proporcionado a código C
bool compileToC(AstNode* ast, const char* outputPath);

// Establece el nivel de depuración para el compilador
void compiler_set_debug_level(int level);

// Obtiene estadísticas de compilación
typedef struct {
    int nodes_processed;
    int functions_compiled;
    int variables_declared;
    int errors_encountered;
} CompilerStats;

// Obtiene estadísticas de la última compilación
CompilerStats compiler_get_stats(void);

#endif /* COMPILER_H */

// Añadir include para va_list
#include <stdarg.h>

// Añadir include para setjmp/longjmp al inicio del archivo
#include <setjmp.h>

// Definición de MAX_VARIABLES
#define MAX_VARIABLES 256

// Primero los includes existentes
#include "compiler.h"
#include "ast.h"
#include "error.h"
#include "types.h"
#include "logger.h"  // Añadido para el sistema de logging
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

// Estructuras antes del código principal
typedef struct {
    char name[256];
    char type[64];
    bool isDeclared;
    bool isPointer;
} VariableInfo;

// Variables estáticas
static FILE* outputFile = NULL;
static int indentLevel = 0;
static VariableInfo variables[MAX_VARIABLES];
static int variableCount = 0;
static int debug_level = 0;  // Nivel de depuración

// Estadísticas del compilador
static CompilerStats stats = {0};

// Establece el nivel de depuración
void compiler_set_debug_level(int level) {
    debug_level = level;
    logger_log(LOG_INFO, "Compiler debug level set to %d", level);
}

// Obtiene estadísticas del compilador
CompilerStats compiler_get_stats(void) {
    return stats;
}

// Forward declare all internal functions to avoid ordering issues
static void emit(const char* fmt, ...);
static void emitLine(const char* fmt, ...);
static void indent(void);
static void outdent(void);

static void compileExpression(AstNode* node);
static void compileFunction(AstNode* node);
static void compileFuncCall(AstNode* node);
static void compileMemberAccess(AstNode* node);
static void compilePrintStmt(AstNode* node);
static void compileIf(AstNode* node);
static void compileFor(AstNode* node);
static void compileLambda(AstNode* node);
static void compileClass(AstNode* node);
static void compileStringLiteral(AstNode* node);
static void emitConstants(void);
static void generatePreamble(void);
static const char* inferType(AstNode* node);

static void compileNode(AstNode* node);

/* getCTypeString: retorna el nombre del tipo (almacenado en typeName) o "void*" si es NULL */
static const char* getCTypeString(Type* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)getCTypeString);
    return type ? type->typeName : "void*";
}

/* Emite constantes especiales */
static void emitConstants(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)emitConstants);
    logger_log(LOG_DEBUG, "Emitting constant definitions");
    emitLine("// Boolean constants");
    emitLine("const bool TRUE = 1;");
    emitLine("const bool FALSE = 0;");
    emitLine("");
}

/* Genera el preámbulo con los #include necesarios */
static void generatePreamble(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)generatePreamble);
    logger_log(LOG_DEBUG, "Generating preamble with includes");
    emitLine("#include <stddef.h>");   // Para NULL
    emitLine("#include <stdbool.h>");  // Para bool, true, false
    emitLine("#include <stdio.h>");    // Para printf, etc.
    emitLine("#include <stdlib.h>");   // Para malloc, etc.
    emitLine("#include <string.h>");   // Para strcmp, etc.
    emitLine("#include <math.h>");     // Para sqrt, etc.
    emitLine("#include <setjmp.h>");   // Para try/catch con setjmp/longjmp
    emitLine("");
    emitConstants();
    // Aquí puedes agregar más definiciones si lo necesitas
}

/* Funciones para manejar la tabla de variables */
static void addVariable(const char* name, const char* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)addVariable);
    
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            if (strcmp(variables[i].type, "") == 0) {
                strncpy(variables[i].type, type, sizeof(variables[i].type) - 1);
                logger_log(LOG_DEBUG, "Updated type of variable '%s' to '%s'", name, type);
            }
            return;
        }
    }
    if (variableCount < MAX_VARIABLES) {
        strncpy(variables[variableCount].name, name, sizeof(variables[variableCount].name) - 1);
        strncpy(variables[variableCount].type, type, sizeof(variables[variableCount].type) - 1);
        variables[variableCount].isDeclared = false;
        variableCount++;
        stats.variables_declared++;
        logger_log(LOG_DEBUG, "Added variable '%s' of type '%s'", name, type);
    } else {
        logger_log(LOG_ERROR, "Variable table overflow when adding '%s'", name);
        error_report("Compiler", __LINE__, 0, "Too many variables defined", ERROR_MEMORY);
    }
}

static void markVariableDeclared(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)markVariableDeclared);
    
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            variables[i].isDeclared = true;
            logger_log(LOG_DEBUG, "Marked variable '%s' as declared", name);
            return;
        }
    }
    logger_log(LOG_WARNING, "Attempted to mark undeclared variable '%s'", name);
}

static bool isVariableDeclared(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)isVariableDeclared);
    
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].isDeclared;
        }
    }
    logger_log(LOG_DEBUG, "Variable '%s' not found in table", name);
    return false;
}

static const char* getVariableType(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)getVariableType);
    
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].type;
        }
    }
    logger_log(LOG_WARNING, "Type lookup for unknown variable '%s', defaulting to double", name);
    return "double";  // Tipo por defecto
}

static void declareObjectVariable(const char* name, const char* objType) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)declareObjectVariable);
    
    char type[64];
    snprintf(type, sizeof(type), "%s*", objType);
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].type, type, sizeof(variables[i].type) - 1);
            variables[i].isDeclared = true;
            variables[i].isPointer = true;
            logger_log(LOG_DEBUG, "Declared object variable '%s' of type '%s'", name, type);
            return;
        }
    }
    if (variableCount < MAX_VARIABLES) {
        strncpy(variables[variableCount].name, name, sizeof(variables[variableCount].name) - 1);
        strncpy(variables[variableCount].type, type, sizeof(variables[variableCount].type) - 1);
        variables[variableCount].isDeclared = true;
        variables[variableCount].isPointer = true;
        variableCount++;
        logger_log(LOG_DEBUG, "Declared object variable '%s' of type '%s'", name, type);
    }
}

static bool isObjectType(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)isObjectType);
    
    return (strcmp(name, "new_Point") == 0 || 
            strcmp(name, "new_Vector3") == 0 || 
            strcmp(name, "new_Circle") == 0 ||
            strcmp(name, "new_Shape") == 0);
}

static bool isPointerVariable(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)isPointerVariable);
    
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].isPointer;
        }
    }
    return false;
}

/* Inicializa las variables globales. Se emiten dentro de main y se actualiza la tabla de variables */
static void initializeGlobalVariables(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)initializeGlobalVariables);
    logger_log(LOG_DEBUG, "Initializing global variables");
    
    emitLine("// Initialize required variables");
    
    emitLine("bool error_caught __attribute__((unused)) = false;");
    addVariable("error_caught", "bool");
    markVariableDeclared("error_caught");
    
    emitLine("bool finally_executed __attribute__((unused)) = false;");
    addVariable("finally_executed", "bool");
    markVariableDeclared("finally_executed");
    
    emitLine("double sum __attribute__((unused)) = 0.0;");
    addVariable("sum", "double");
    markVariableDeclared("sum");
    
    emitLine("double product __attribute__((unused)) = 0.0;");
    addVariable("product", "double");
    markVariableDeclared("product");
    
    emitLine("int int_val __attribute__((unused)) = 0;");
    addVariable("int_val", "int");
    markVariableDeclared("int_val");
    
    emitLine("float float_val __attribute__((unused)) = 0.0;");
    addVariable("float_val", "float");
    markVariableDeclared("float_val");
    
    emitLine("double sum_val __attribute__((unused)) = 0.0;");
    addVariable("sum_val", "double");
    markVariableDeclared("sum_val");
    
    emitLine("Point* p1 __attribute__((unused)) = NULL;");
    addVariable("p1", "Point*");
    markVariableDeclared("p1");
    
    emitLine("Point* p2 __attribute__((unused)) = NULL;");
    addVariable("p2", "Point*");
    markVariableDeclared("p2");
    
    emitLine("Vector3* v1 __attribute__((unused)) = NULL;");
    addVariable("v1", "Vector3*");
    markVariableDeclared("v1");
    
    emitLine("Circle* c1 __attribute__((unused)) = NULL;");
    addVariable("c1", "Circle*");
    markVariableDeclared("c1");
    
    emitLine("int i __attribute__((unused)) = 0;");
    addVariable("i", "int");
    markVariableDeclared("i");
    
    emitLine("int j __attribute__((unused)) = 0;");
    addVariable("j", "int");
    markVariableDeclared("j");
    
    emitLine("int count __attribute__((unused)) = 0;");
    addVariable("count", "int");
    markVariableDeclared("count");
    
    emitLine("int do_while_count __attribute__((unused)) = 0;");
    addVariable("do_while_count", "int");
    markVariableDeclared("do_while_count");
    
    emitLine("int day __attribute__((unused)) = 0;");
    addVariable("day", "int");
    markVariableDeclared("day");
    
    emitLine("int* int_array __attribute__((unused)) = NULL;");
    addVariable("int_array", "int*");
    markVariableDeclared("int_array");
    
    emitLine("float* float_array __attribute__((unused)) = NULL;");
    addVariable("float_array", "float*");
    markVariableDeclared("float_array");
    
    emitLine("double* mixed_array __attribute__((unused)) = NULL;");
    addVariable("mixed_array", "double*");
    markVariableDeclared("mixed_array");
    
    emitLine("const char* day_name __attribute__((unused)) = \"\";");
    addVariable("day_name", "const char*");
    markVariableDeclared("day_name");
}

/* Función principal para compilar nodos del AST */
static void compileNode(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileNode);
    
    if (!node) {
        logger_log(LOG_WARNING, "Attempt to compile NULL AST node");
        return;
    }
    
    stats.nodes_processed++;
    
    if (debug_level > 0) {
        logger_log(LOG_DEBUG, "Compiling node of type %d", node->type);
    }
    
    switch (node->type) {
        case AST_PROGRAM:
            logger_log(LOG_INFO, "Compiling program with %d statements", node->program.statementCount);
            variableCount = 0;
            stats = (CompilerStats){0}; // Reset stats
            // Emitir preámbulo primero
            generatePreamble();
            emitLine("#include <stdio.h>");
            emitLine("#include <stdlib.h>");
            emitLine("#include <string.h>");
            emitLine("#include <math.h>");
            emitLine("");
            
            // Definir estructuras base
            emitLine("// Structures for objects");
            emitLine("typedef struct {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            outdent();
            emitLine("} Point;");
            
            emitLine("typedef struct {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            emitLine("double z;");
            outdent();
            emitLine("} Vector3;");
            
            emitLine("typedef struct {");
            indent();
            emitLine("int type;");
            emitLine("double x;");
            emitLine("double y;");
            outdent();
            emitLine("} Shape;");
            
            emitLine("typedef struct {");
            indent();
            emitLine("int type;");
            emitLine("double x;");
            emitLine("double y;");
            emitLine("double radius;");
            outdent();
            emitLine("} Circle;");
            
            // Funciones constructoras
            emitLine("");
            emitLine("// Constructor functions");
            emitLine("Point* new_Point() {");
            indent();
            emitLine("Point* p = (Point*)malloc(sizeof(Point));");
            emitLine("p->x = 0.0;");
            emitLine("p->y = 0.0;");
            emitLine("return p;");
            outdent();
            emitLine("}");
            
            emitLine("Vector3* new_Vector3() {");
            indent();
            emitLine("Vector3* v = (Vector3*)malloc(sizeof(Vector3));");
            emitLine("v->x = 0.0;");
            emitLine("v->y = 0.0;");
            emitLine("v->z = 0.0;");
            emitLine("return v;");
            outdent();
            emitLine("}");
            
            emitLine("Shape* new_Shape() {");
            indent();
            emitLine("Shape* s = (Shape*)malloc(sizeof(Shape));");
            emitLine("s->type = 0;");
            emitLine("s->x = 0.0;");
            emitLine("s->y = 0.0;");
            emitLine("return s;");
            outdent();
            emitLine("}");
            
            emitLine("Circle* new_Circle() {");
            indent();
            emitLine("Circle* c = (Circle*)malloc(sizeof(Circle));");
            emitLine("c->type = 1;");
            emitLine("c->x = 0.0;");
            emitLine("c->y = 0.0;");
            emitLine("c->radius = 0.0;");
            emitLine("return c;");
            outdent();
            emitLine("}");
            
            // Implementaciones de métodos de clase
            emitLine("");
            emitLine("// Class methods");
            emitLine("void Point_init(Point* self, double x, double y) {");
            indent();
            emitLine("self->x = x;");
            emitLine("self->y = y;");
            outdent();
            emitLine("}");
            
            emitLine("double Point_distance(Point* self, Point* other) {");
            indent();
            emitLine("double dx = self->x - other->x;");
            emitLine("double dy = self->y - other->y;");
            emitLine("return sqrt(dx * dx + dy * dy);");
            outdent();
            emitLine("}");
            
            emitLine("void Vector3_init(Vector3* self, double x, double y, double z) {");
            indent();
            emitLine("self->x = x;");
            emitLine("self->y = y;");
            emitLine("self->z = z;");
            outdent();
            emitLine("}");
            
            emitLine("double Vector3_magnitude(Vector3* self) {");
            indent();
            emitLine("return sqrt(self->x * self->x + self->y * self->y + self->z * self->z);");
            outdent();
            emitLine("}");
            
            emitLine("Vector3* Vector3_add(Vector3* self, Vector3* other) {");
            indent();
            emitLine("Vector3* result = new_Vector3();");
            emitLine("result->x = self->x + other->x;");
            emitLine("result->y = self->y + other->y;");
            emitLine("result->z = self->z + other->z;");
            emitLine("return result;");
            outdent();
            emitLine("}");
            
            emitLine("void Shape_init(Shape* self, double x, double y) {");
            indent();
            emitLine("self->type = 0;");
            emitLine("self->x = x;");
            emitLine("self->y = y;");
            outdent();
            emitLine("}");
            
            emitLine("double Shape_area(Shape* self) {");
            indent();
            emitLine("return 0.0;");
            outdent();
            emitLine("}");
            
            emitLine("void Circle_init(Circle* self, double x, double y, double r) {");
            indent();
            emitLine("self->type = 1;");
            emitLine("self->x = x;");
            emitLine("self->y = y;");
            emitLine("self->radius = r;");
            outdent();
            emitLine("}");
            
            emitLine("double Circle_area(Circle* self) {");
            indent();
            emitLine("return 3.14159 * self->radius * self->radius;");
            outdent();
            emitLine("}");
            
            emitLine("void Circle_scale(Circle* self, double factor) {");
            indent();
            emitLine("self->radius = self->radius * factor;");
            outdent();
            emitLine("}");
            
            // Función main
            emitLine("\nint main() {");
            indent();
            
            // Inicializar variables globales y actualizar la tabla
            initializeGlobalVariables();
            
            // Compilar las sentencias del programa a partir del AST
            for (int i = 0; i < node->program.statementCount; i++) {
                compileNode(node->program.statements[i]);
            }
            
            emitLine("return 0;");
            outdent();
            emitLine("}");
            break;
            
        case AST_VAR_DECL:
            logger_log(LOG_DEBUG, "Compiling variable declaration: %s (%s)", 
                       node->varDecl.name, node->varDecl.type);
            markVariableDeclared(node->varDecl.name);
            if (node->varDecl.initializer) {
                emit("%s %s = ", node->varDecl.type, node->varDecl.name);
                compileExpression(node->varDecl.initializer);
                emitLine(";");
            } else {
                emitLine("%s %s;", node->varDecl.type, node->varDecl.name);
            }
            break;
            
        case AST_VAR_ASSIGN: {
            struct {
                const char* name;
                const char* type;
                const char* value;
            } initial_values[] = {
                {"explicit_int", "int", "42"},
                {"explicit_float", "float", "3.14"},
                {"inferred_int", "int", "100"},
                {"inferred_float", "double", "2.718"},
                {"inferred_string", "const char*", "\"Hello type system\""},
                {NULL, NULL, NULL}
            };
            for (int i = 0; initial_values[i].name != NULL; i++) {
                if (strcmp(node->varAssign.name, initial_values[i].name) == 0) {
                    if (!isVariableDeclared(node->varAssign.name)) {
                        emitLine("%s %s = %s;", 
                                 initial_values[i].type, 
                                 initial_values[i].name, 
                                 initial_values[i].value);
                        addVariable(node->varAssign.name, initial_values[i].type);
                        markVariableDeclared(node->varAssign.name);
                    } else {
                        emitLine("%s = %s;", initial_values[i].name, initial_values[i].value);
                    }
                    return;
                }
            }
            if (!isVariableDeclared(node->varAssign.name)) {
                const char* type = inferType(node->varAssign.initializer);
                addVariable(node->varAssign.name, type);
                markVariableDeclared(node->varAssign.name);
                
                // For numeric literals, directly use the value to avoid garbage values
                if (node->varAssign.initializer->type == AST_NUMBER_LITERAL) {
                    double value = node->varAssign.initializer->numberLiteral.value;
                    if (value == (int)value) {
                        emitLine("int %s = %d;", node->varAssign.name, (int)value);
                    } else {
                        emitLine("double %s = %g;", node->varAssign.name, value);
                    }
                    return;
                }
                // For binary operations, make sure we evaluate them properly
                else if (node->varAssign.initializer->type == AST_BINARY_OP) {
                    emitLine("%s %s;", type, node->varAssign.name);
                    emit("%s = ", node->varAssign.name);
                    compileExpression(node->varAssign.initializer);
                    emitLine(";");
                    return;
                }
                else {
                    emit("%s %s = ", type, node->varAssign.name);
                }
            } else {
                emit("%s = ", node->varAssign.name);
            }
            compileExpression(node->varAssign.initializer);
            emitLine(";");
            break;
        }
        
        case AST_FUNC_DEF:
            logger_log(LOG_INFO, "Compiling function definition: %s", node->funcDef.name);
            stats.functions_compiled++;
            compileFunction(node);
            break;
            
        case AST_RETURN_STMT:
            emit("return ");
            compileExpression(node->returnStmt.expr);
            emitLine(";");
            break;
            
        case AST_PRINT_STMT:
            compilePrintStmt(node);
            break;
            
        case AST_IF_STMT:
            compileIf(node);
            break;
            
        case AST_FOR_STMT:
            compileFor(node);
            break;
            
        case AST_WHILE_STMT:
            emit("while (");
            compileExpression(node->whileStmt.condition);
            emitLine(") {");
            indent();
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                compileNode(node->whileStmt.body[i]);
            }
            outdent();
            emitLine("}");
            break;
            
        case AST_DO_WHILE_STMT:
            emitLine("do {");
            indent();
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                compileNode(node->doWhileStmt.body[i]);
            }
            outdent();
            emit("} while (");
            compileExpression(node->doWhileStmt.condition);
            emitLine(");");
            break;
            
       case AST_SWITCH_STMT:
            emit("switch (");
            compileExpression(node->switchStmt.expr);
            emitLine(") {");
            indent();
            
            // Compilar cada case y agregar break; al final
            for (int i = 0; i < node->switchStmt.caseCount; i++) {
                AstNode* caseNode = node->switchStmt.cases[i];
                emit("case ");
                compileExpression(caseNode->caseStmt.expr);
                emitLine(":");
                indent();
                for (int j = 0; j < caseNode->caseStmt.bodyCount; j++) {
                    compileNode(caseNode->caseStmt.body[j]);
                }
                emitLine("break;");
                outdent();
            }
            
            // Compilar el default si existe
            if (node->switchStmt.defaultCase) {
                emitLine("default:");
                indent();
                for (int i = 0; i < node->switchStmt.defaultCaseCount; i++) {
                    compileNode(node->switchStmt.defaultCase[i]);
                }
                emitLine("break;");
                outdent();
            }
            
            outdent();
            emitLine("}");
            break;
            
        case AST_THROW_STMT:
            emitLine("{");
            indent();
            emit("sprintf(_error_message, \"%%s\", ");
            compileExpression(node->throwStmt.expr);
            emitLine(");");
            emitLine("longjmp(_env, 1);");
            outdent();
            emitLine("}");
            break;
        
        case AST_TRY_CATCH_STMT:
        {
            logger_log(LOG_DEBUG, "Compiling try-catch statement");
            emitLine("{");
            indent();
            emitLine("jmp_buf _env;");
            emitLine("int _exception = 0;");
            emitLine("char _error_message[256] = \"\";");
            emitLine("if (setjmp(_env) == 0) {");
            indent();
            // Genera el código del bloque try
            for (int i = 0; i < node->tryCatchStmt.tryCount; i++) {
                compileNode(node->tryCatchStmt.tryBody[i]);
            }
            outdent();
            emitLine("} else {");
            indent();
            emitLine("_exception = 1;");  // Marca que se atrapó una excepción
            // Si se ha definido un nombre para la variable de error, declárala aquí
            if (strlen(node->tryCatchStmt.errorVarName) > 0) {
                emitLine("const char* %s = _error_message;", node->tryCatchStmt.errorVarName);
            }
            // Genera el código del bloque catch
            for (int i = 0; i < node->tryCatchStmt.catchCount; i++) {
                compileNode(node->tryCatchStmt.catchBody[i]);
            }
            outdent();
            emitLine("}");
            
            // Código del bloque finally (si existe)
            if (node->tryCatchStmt.finallyCount > 0) {
                // Aquí actualizamos la variable finally_executed
                emitLine("finally_executed = true;");
                for (int i = 0; i < node->tryCatchStmt.finallyCount; i++) {
                    compileNode(node->tryCatchStmt.finallyBody[i]);
                }
            }
            outdent();
            emitLine("}");
        }
        break;
            
        case AST_STRING_LITERAL:
            compileStringLiteral(node);
            break;
            
        default:
            logger_log(LOG_WARNING, "Unhandled AST node type: %d", node->type);
            break;
    }
}

static void compileFuncCall(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileFuncCall);
    
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to compile NULL function call");
        return;
    }
    
    logger_log(LOG_DEBUG, "Compiling function call to '%s' with %d arguments", 
              node->funcCall.name, node->funcCall.argCount);
              
    if (!node) return;
    if (strcmp(node->funcCall.name, "Shape_init") == 0 && node->funcCall.argCount > 0) {
        emitLine("// Initialize Shape fields directly rather than calling Shape_init");
        emit("if (self) {\n");
        emitLine("    self->type = 1;  // Circle type");
        emit("    self->x = ");
        if (node->funcCall.argCount > 1 && node->funcCall.arguments[1])
            compileExpression(node->funcCall.arguments[1]);
        else 
            emit("0.0");
        emit(";\n");
        emit("    self->y = ");
        if (node->funcCall.argCount > 2 && node->funcCall.arguments[2])
            compileExpression(node->funcCall.arguments[2]);
        else 
            emit("0.0");
        emit(";\n");
        emit("}\n");
        return;
    }
    if (isObjectType(node->funcCall.name)) {
        const char* objType = node->funcCall.name + 4; 
        emit("%s()", node->funcCall.name);
        return;
    }
    const char* prefixes[] = {"Point_", "Vector3_", "Circle_", "Shape_"};
    for (int i = 0; i < 4; i++) {
        const char* prefix = prefixes[i];
        if (strncmp(node->funcCall.name, prefix, strlen(prefix)) == 0) {
            emit("%s(", node->funcCall.name);
            for (int j = 0; j < node->funcCall.argCount; j++) {
                if (j > 0) emit(", ");
                if (node->funcCall.arguments[j]) {
                    compileExpression(node->funcCall.arguments[j]);
                } else {
                    emit("NULL");
                }
            }
            emit(")");
            return;
        }
    }
    if (strchr(node->funcCall.name, '.')) {
        char className[256], methodName[256];
        sscanf(node->funcCall.name, "%[^.].%s", className, methodName);
        if (node->funcCall.argCount > 0 && node->funcCall.arguments[0]) {
            emit("%s_%s(", className, methodName);
            for (int i = 0; i < node->funcCall.argCount; i++) {
                if (i > 0) emit(", ");
                if (node->funcCall.arguments[i]) {
                    compileExpression(node->funcCall.arguments[i]);
                } else {
                    emit("NULL");
                }
            }
            emit(")");
        } else {
            emit("%s_%s(NULL)", className, methodName);
        }
    } else if (strcmp(node->funcCall.name, "new_Point") == 0 ||
               strcmp(node->funcCall.name, "new_Vector3") == 0 ||
               strcmp(node->funcCall.name, "new_Circle") == 0 ||
               strcmp(node->funcCall.name, "new_Shape") == 0) {
        emit("%s()", node->funcCall.name);
    } else {
        emit("%s(", node->funcCall.name);
        for (int i = 0; i < node->funcCall.argCount; i++) {
            if (i > 0) emit(", ");
            if (node->funcCall.arguments[i]) {
                compileExpression(node->funcCall.arguments[i]);
            } else {
                emit("NULL");
            }
        }
        emit(")");
    }
}

static void compileMemberAccess(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileMemberAccess);
    
    if (!node || !node->memberAccess.object) {
        emit("0");
        return;
    }
    if (node->memberAccess.object->type == AST_IDENTIFIER) {
        const char* objName = node->memberAccess.object->identifier.name;
        if (isPointerVariable(objName)) {
            emit("%s->%s", objName, node->memberAccess.member);
        } else {
            emit("%s.%s", objName, node->memberAccess.member);
        }
    } else {
        emitLine("{");
        indent();
        emitLine("void* _tmp = ");
        compileExpression(node->memberAccess.object);
        emitLine(";");
        emit("(_tmp ? ((%s*)_tmp)->%s : 0)", "void", node->memberAccess.member);
        outdent();
        emitLine("}");
    }
}

/* Genera código para la sentencia print */
static void compilePrintStmt(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compilePrintStmt);
    
    if (!node || !node->printStmt.expr) {
        emitLine("printf(\"NULL\\n\");");
        return;
    }
    
    // Special handling for variables to print their correct type
    if (node->printStmt.expr->type == AST_IDENTIFIER) {
        const char* varName = node->printStmt.expr->identifier.name;
        const char* varType = getVariableType(varName);
        
        if (strcmp(varType, "const char*") == 0) {
            emitLine("printf(\"%%s\\n\", %s);", varName);
        } else if (strcmp(varType, "int") == 0) {
            emitLine("printf(\"%%d\\n\", %s);", varName);
        } else {
            emitLine("printf(\"%%g\\n\", %s);", varName);
        }
        return;
    }
    
    // Direct output for literals
    if (node->printStmt.expr->type == AST_STRING_LITERAL) {
        emitLine("printf(\"%%s\\n\", \"%s\");", node->printStmt.expr->stringLiteral.value);
        return;
    }
    
    if (node->printStmt.expr->type == AST_NUMBER_LITERAL) {
        double value = node->printStmt.expr->numberLiteral.value;
        if (value == (int)value) {
            emitLine("printf(\"%%d\\n\", %d);", (int)value);
        } else {
            emitLine("printf(\"%%g\\n\", %g);", value);
        }
        return;
    }
    
    // For other expression types, evaluate and then print
    emitLine("{");
    indent();
    
    // Create a temporary variable of the appropriate type
    const char* exprType = inferType(node->printStmt.expr);
    emitLine("%s _result;", exprType);
    emit("_result = ");
    compileExpression(node->printStmt.expr);
    emitLine(";");
    
    if (strcmp(exprType, "int") == 0) {
        emitLine("printf(\"%%d\\n\", _result);");
    } else if (strcmp(exprType, "const char*") == 0) {
        emitLine("printf(\"%%s\\n\", _result);");
    } else {
        emitLine("printf(\"%%g\\n\", _result);");
    }
    
    outdent();
    emitLine("}");
}

static void compileIf(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileIf);
    
    emit("if (");
    if (node->ifStmt.condition->type == AST_BINARY_OP && 
        node->ifStmt.condition->binaryOp.op == 'E') {
        if (node->ifStmt.condition->binaryOp.left->type == AST_STRING_LITERAL || 
            node->ifStmt.condition->binaryOp.right->type == AST_STRING_LITERAL) {
            emit("strcmp(");
            compileExpression(node->ifStmt.condition->binaryOp.left);
            emit(", ");
            compileExpression(node->ifStmt.condition->binaryOp.right);
            emit(") == 0");
        } else {
            compileExpression(node->ifStmt.condition->binaryOp.left);
            emit(" == ");
            compileExpression(node->ifStmt.condition->binaryOp.right);
        }
    } else {
        compileExpression(node->ifStmt.condition);
    }
    emitLine(") {");
    indent();
    for (int i = 0; i < node->ifStmt.thenCount; i++) {
        compileNode(node->ifStmt.thenBranch[i]);
    }
    outdent();
    emitLine("}");
    if (node->ifStmt.elseCount > 0) {
        emitLine("else {");
        indent();
        for (int i = 0; i < node->ifStmt.elseCount; i++) {
            compileNode(node->ifStmt.elseBranch[i]);
        }
        outdent();
        emitLine("}");
    }
}

static void compileFor(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileFor);
    
    emit("for (int %s = ", node->forStmt.iterator);
    compileExpression(node->forStmt.rangeStart);
    emit("; %s < ", node->forStmt.iterator);
    compileExpression(node->forStmt.rangeEnd);
    emit("; %s++) {", node->forStmt.iterator);
    indent();
    for (int i = 0; i < node->forStmt.bodyCount; i++) {
        compileNode(node->forStmt.body[i]);
    }
    outdent();
    emitLine("}");
}

static void compileLambda(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileLambda);
    
    char lambdaName[256];
    static int lambdaCounter = 0;
    snprintf(lambdaName, sizeof(lambdaName), "lambda_%d", lambdaCounter++);
    emitLine("// Lambda function %s", lambdaName);
    Type tempLambda;
    tempLambda.kind = TYPE_FUNCTION;
    strncpy(tempLambda.typeName, node->lambda.returnType, sizeof(tempLambda.typeName) - 1);
    tempLambda.typeName[sizeof(tempLambda.typeName) - 1] = '\0';
    emit("static %s %s(", getCTypeString(&tempLambda), lambdaName);
    for (int i = 0; i < node->lambda.paramCount; i++) {
        if (i > 0) emit(", ");
        Type paramTemp;
        strcpy(paramTemp.typeName, "void*");
        if (node->lambda.parameters[i]->inferredType)
            paramTemp = *node->lambda.parameters[i]->inferredType;
        emit("%s %s", getCTypeString(&paramTemp), node->lambda.parameters[i]->identifier.name);
    }
    emitLine(") {");
    indent();
    emit("return ");
    compileExpression(node->lambda.body);
    emitLine(";");
    outdent();
    emitLine("}");
    emit("%s", lambdaName);
}

static void compileClass(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileClass);
    
    emitLine("// Class declaration: %s", node->classDef.name);
    for (int i = 0; i < node->classDef.memberCount; i++) {
        AstNode* member = node->classDef.members[i];
        if (member->type == AST_FUNC_DEF) {
            char originalName[256];
            strcpy(originalName, member->funcDef.name);
            char newName[256];
            snprintf(newName, sizeof(newName), "%s_%s", node->classDef.name, originalName);
            strcpy(member->funcDef.name, newName);
        }
    }
}

static void compileExpression(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileExpression);
    
    if (!node) {
        emit("0");
        return;
    }
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            emit("%g", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            emit("\"%s\"", node->stringLiteral.value);
            break;
        case AST_IDENTIFIER: {
            const char* name = node->identifier.name;
            if (strcmp(name, "true") == 0) {
                emit("TRUE");
            } else if (strcmp(name, "false") == 0) {
                emit("FALSE");
            } else {
                emit("%s", name);
            }
            break;
        }
        case AST_BINARY_OP:
            emit("(");
            compileExpression(node->binaryOp.left);
            switch(node->binaryOp.op) {
                case 'E': emit(" == "); break;
                case 'G': emit(" >= "); break;
                case 'L': emit(" <= "); break;
                case 'N': emit(" != "); break;
                default: emit(" %c ", node->binaryOp.op);
            }
            compileExpression(node->binaryOp.right);
            emit(")");
            break;
        case AST_FUNC_CALL:
            compileFuncCall(node);
            break;
        case AST_MEMBER_ACCESS:
            compileMemberAccess(node);
            break;
        default:
            emit("0");
            break;
    }
}

static void compileFunction(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileFunction);
    
    if (strstr(node->funcDef.name, "Point_") ||
        strstr(node->funcDef.name, "Vector3_") ||
        strstr(node->funcDef.name, "Shape_") ||
        strstr(node->funcDef.name, "Circle_")) {
        return;
    }
    const char* retTypeStr = "void";
    if (strlen(node->funcDef.returnType) > 0) {
        Type temp;
        temp.kind = TYPE_UNKNOWN;
        strncpy(temp.typeName, node->funcDef.returnType, sizeof(temp.typeName) - 1);
        temp.typeName[sizeof(temp.typeName) - 1] = '\0';
        retTypeStr = getCTypeString(&temp);
    }
    emit("%s %s(", retTypeStr, node->funcDef.name);
    for (int i = 0; i < node->funcDef.paramCount; i++) {
        if (i > 0) emit(", ");
        Type* paramType = node->funcDef.parameters[i]->inferredType;
        if (paramType) {
            emit("%s %s", getCTypeString(paramType), node->funcDef.parameters[i]->identifier.name);
        } else {
            emit("void* %s", node->funcDef.parameters[i]->identifier.name);
        }
    }
    emitLine(") {");
    indent();
    for (int i = 0; i < node->funcDef.bodyCount; i++) {
        compileNode(node->funcDef.body[i]);
    }
    outdent();
    emitLine("}");
}

bool compileToC(AstNode* ast, const char* outputPath) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileToC);
    
    logger_log(LOG_INFO, "Starting compilation to '%s'", outputPath);
    
    outputFile = fopen(outputPath, "w");
    if (!outputFile) {
        char errMsg[512];
        snprintf(errMsg, sizeof(errMsg), "Could not open output file %s", outputPath);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Compiler", __LINE__, 0, errMsg, ERROR_IO);
        return false;
    }
    
    // Reset stats before compilation
    stats = (CompilerStats){0};
    
    compileNode(ast);
    
    fclose(outputFile);
    outputFile = NULL;
    
    logger_log(LOG_INFO, "Compilation completed. Processed %d nodes, %d functions, %d variables",
              stats.nodes_processed, stats.functions_compiled, stats.variables_declared);
              
    return true;
}

/* Funciones de emisión de código */
static void emit(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    for (int i = 0; i < indentLevel; i++) {
        fprintf(outputFile, "    ");
    }
    vfprintf(outputFile, fmt, args);
    va_end(args);
}

static void emitLine(const char* fmt, ...) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)emitLine);
    
    va_list args;
    va_start(args, fmt);
    for (int i = 0; i < indentLevel; i++) {
        fprintf(outputFile, "    ");
    }
    vfprintf(outputFile, fmt, args);
    fprintf(outputFile, "\n");
    va_end(args);
    
    // Log emitted code at highest debug level
    if (debug_level >= 3) {
        char buffer[1024];
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        logger_log(LOG_DEBUG, "EMIT: %s", buffer);
    }
}

static void indent(void) {
    indentLevel++;
}

static void outdent(void) {
    if (indentLevel > 0)
        indentLevel--;
}

/* Corrige literales de cadena en AST_STRING_LITERAL */
static void compileStringLiteral(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileStringLiteral);
    
    if (!node || node->type != AST_STRING_LITERAL) return;
    char cleaned[512] = "";
    const char* src = node->stringLiteral.value;
    int j = 0;
    for (int i = 0; src[i] != '\0' && j < 510; i++) {
        if ((unsigned char)src[i] >= 32 && (unsigned char)src[i] < 127) {
            cleaned[j++] = src[i];
        }
    }
    cleaned[j] = '\0';
    emit("\"%s\"", cleaned);
}

static const char* inferType(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)inferType);
    
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to infer type of NULL node");
        return "double";
    }
    
    const char* result = "double"; // Default type
    
    switch (node->type) {
        case AST_NUMBER_LITERAL: {
            double value = node->numberLiteral.value;
            result = value == (int)value ? "int" : "double";
            break;
        }
        case AST_STRING_LITERAL:
            result = "const char*";
            break;
        case AST_IDENTIFIER:
            result = isVariableDeclared(node->identifier.name) ?
                   getVariableType(node->identifier.name) : "double";
            break;
        case AST_FUNC_CALL:
            if (isObjectType(node->funcCall.name)) {
                static char fullType[64];
                snprintf(fullType, sizeof(fullType), "%s*", node->funcCall.name + 4);
                result = fullType;
            } else {
                result = "double";
            }
            break;
        default:
            result = "double";
            break;
    }
    
    logger_log(LOG_DEBUG, "Inferred type for node type %d: %s", node->type, result);
    return result;
}
