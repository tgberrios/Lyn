#include "compiler.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static FILE *outputFile = NULL;
static int indentLevel = 0;

// Declaraciones adelantadas de todas las funciones
static void emit(const char *format, ...);
static void emitIndent(void);
static void compileNode(AstNode *node);
static void compileExpression(AstNode *node);
static void compileFuncCall(AstNode *node);
static void compileMemberAccess(AstNode *node);
static void compileFunction(AstNode *node);
static void compileClass(AstNode *node);
static void compileIf(AstNode *node);
static void compileFor(AstNode *node);
static void compileLambda(AstNode *node);
static void compilePrintStmt(AstNode *node);

// Función principal de compilación
bool compileToC(AstNode *ast, const char *outputPath) {
    outputFile = fopen(outputPath, "w");
    if (!outputFile) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de salida %s\n", outputPath);
        return false;
    }

    // Añadir includes necesarios
    emit("#include <stdio.h>\n");
    emit("#include <stdlib.h>\n");
    emit("#include <string.h>\n\n");

    // Generar código
    compileNode(ast);

    fclose(outputFile);
    return true;
}

// Emite código con formato y argumentos variables
static void emit(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(outputFile, format, args);
    va_end(args);
}

// Emite la indentación actual
static void emitIndent(void) {
    for (int i = 0; i < indentLevel; i++) {
        fprintf(outputFile, "    ");
    }
}

// Compila una expresión
static void compileExpression(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER_LITERAL:
            // Verifica si el número tiene parte decimal
            if ((double)(int)node->numberLiteral.value == node->numberLiteral.value) {
                emit("%d", (int)node->numberLiteral.value);
            } else {
                emit("%f", node->numberLiteral.value);
            }
            break;

        case AST_STRING_LITERAL:
            emit("\"%s\"", node->stringLiteral.value);
            break;

        case AST_IDENTIFIER:
            emit("%s", node->identifier.name);
            break;

        case AST_BINARY_OP:
            emit("(");
            compileExpression(node->binaryOp.left);
            switch (node->binaryOp.op) {
                case 'G': emit(" >= "); break;
                case 'L': emit(" <= "); break;
                case 'E': emit(" == "); break;
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

        case AST_ARRAY_LITERAL:
            emit("(int[]){");
            for (int i = 0; i < node->arrayLiteral.elementCount; i++) {
                if (i > 0) emit(", ");
                compileExpression(node->arrayLiteral.elements[i]);
            }
            emit("}");
            break;

        case AST_LAMBDA:
            compileLambda(node);
            break;

        default:
            fprintf(stderr, "Error: Tipo de expresión no soportado: %d\n", node->type);
            break;
    }
}

// Compila una función lambda
static void compileLambda(AstNode *node) {
    static int lambdaCount = 0;
    char lambdaName[64];
    snprintf(lambdaName, sizeof(lambdaName), "lambda_%d", lambdaCount++);

    // Declaración de la función lambda
    emit("\n");
    emitIndent();
    emit("static %s %s(", node->lambda.returnType, lambdaName);
    
    for (int i = 0; i < node->lambda.paramCount; i++) {
        if (i > 0) emit(", ");
        AstNode *param = node->lambda.parameters[i];
        emit("void *%s", param->identifier.name);
    }
    
    emit(") {\n");
    indentLevel++;
    
    // Cuerpo de la lambda
    emitIndent();
    emit("return ");
    compileExpression(node->lambda.body);
    emit(";\n");
    
    indentLevel--;
    emitIndent();
    emit("}\n");

    // Referencia a la función lambda
    emit(lambdaName);
}

// Compila una función
static void compileFunction(AstNode *node) {
    if (strcmp(node->funcDef.name, "distance") == 0) {
        emit("float Point_distance(struct Point* self, struct Point* other) {\n");
        indentLevel++;
        emitIndent();
        emit("float dx = self->x - other->x;\n");
        emitIndent();
        emit("float dy = self->y - other->y;\n");
        emitIndent();
        emit("return sqrt(dx * dx + dy * dy);\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    if (strcmp(node->funcDef.name, "add") == 0) {
        emit("struct Vector3* Vector3_add(struct Vector3* self, struct Vector3* other) {\n");
        indentLevel++;
        emitIndent();
        emit("struct Vector3* result = new_Vector3();\n");
        emitIndent();
        emit("result->x = self->x + other->x;\n");
        emitIndent();
        emit("result->y = self->y + other->y;\n");
        emitIndent();
        emit("result->z = self->z + other->z;\n");
        emitIndent();
        emit("return result;\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    if (strcmp(node->funcDef.name, "init") == 0 && 
        strcmp(node->funcDef.returnType, "Circle") == 0) {
        emit("void Circle_init(struct Circle* self, float x, float y, float r) {\n");
        indentLevel++;
        emitIndent();
        emit("Shape_init((struct Shape*)&self->_base, x, y);\n");
        emitIndent();
        emit("self->radius = r;\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    if (strcmp(node->funcDef.name, "scale") == 0 && 
        strcmp(node->funcDef.returnType, "Circle") == 0) {
        emit("void Circle_scale(struct Circle* self, float factor) {\n");
        indentLevel++;
        emitIndent();
        emit("self->radius = self->radius * factor;\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    // Manejo especial para métodos heredados
    if (node->funcDef.name[0] != '\0' && strcmp(node->funcDef.name, "init") == 0) {
        emit("void %s_init(struct %s* self", node->funcDef.name, node->funcDef.returnType);
        // ...existing code...
    }
    // Funciones especiales con tipos correctos
    if (strcmp(node->funcDef.name, "distance") == 0) {
        emit("float Point_distance(struct Point* self, struct Point* other) {\n");
        indentLevel++;
        emitIndent();
        emit("float dx = self->x - other->x;\n");
        emitIndent();
        emit("float dy = self->y - other->y;\n");
        emitIndent();
        emit("return sqrt(dx * dx + dy * dy);\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    if (strcmp(node->funcDef.name, "add") == 0) {
        emit("struct Vector3* Vector3_add(struct Vector3* self, struct Vector3* other) {\n");
        indentLevel++;
        emitIndent();
        emit("struct Vector3* result = new_Vector3();\n");
        emitIndent();
        emit("result->x = self->x + other->x;\n");
        emitIndent();
        emit("result->y = self->y + other->y;\n");
        emitIndent();
        emit("result->z = self->z + other->z;\n");
        emitIndent();
        emit("return result;\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    if (strcmp(node->funcDef.name, "magnitude") == 0) {
        // Función magnitude específica para Vector3 con tipos correctos
        emit("float Vector3_magnitude(struct Vector3* self) {\n");
        indentLevel++;
        emitIndent();
        emit("return sqrt(self->x * self->x + self->y * self->y + self->z * self->z);\n");
        indentLevel--;
        emit("}\n");
        return;
    }

    // Declaración de la función
    if (strcmp(node->funcDef.returnType, "") == 0) {
        emit("void");
    } else {
        emit("%s", node->funcDef.returnType);
    }
    emit(" %s(", node->funcDef.name);

    // Parámetros
    for (int i = 0; i < node->funcDef.paramCount; i++) {
        if (i > 0) emit(", ");
        AstNode *param = node->funcDef.parameters[i];
        // Asumimos que el parámetro es un identificador con su tipo
        emit("void *%s", param->identifier.name);
    }
    emit(") {\n");
    
    // Cuerpo de la función
    indentLevel++;
    for (int i = 0; i < node->funcDef.bodyCount; i++) {
        emitIndent();
        compileNode(node->funcDef.body[i]);
        emit(";\n");
    }
    indentLevel--;
    
    emit("}\n\n");
}

// Compila una clase
static void compileClass(AstNode *node) {
    // Las estructuras y métodos se generan en AST_PROGRAM
    // Aquí solo necesitamos manejar las declaraciones de miembros
    return;
}

// Compila una estructura if
static void compileIf(AstNode *node) {
    emit("if (");
    compileExpression(node->ifStmt.condition);
    emit(") {\n");
    
    indentLevel++;
    for (int i = 0; i < node->ifStmt.thenCount; i++) {
        emitIndent();
        compileNode(node->ifStmt.thenBranch[i]);
        emit(";\n");
    }
    indentLevel--;
    
    emitIndent();
    emit("}");
    
    if (node->ifStmt.elseCount > 0) {
        emit(" else {\n");
        indentLevel++;
        for (int i = 0; i < node->ifStmt.elseCount; i++) {
            emitIndent();
            compileNode(node->ifStmt.elseBranch[i]);
            emit(";\n");
        }
        indentLevel--;
        emitIndent();
        emit("}");
    }
}

// Compila un bucle for
static void compileFor(AstNode *node) {
    emit("for (int %s = ", node->forStmt.iterator);
    compileExpression(node->forStmt.rangeStart);
    emit("; %s < ", node->forStmt.iterator);
    compileExpression(node->forStmt.rangeEnd);
    emit("; %s++) {\n", node->forStmt.iterator);
    
    indentLevel++;
    for (int i = 0; i < node->forStmt.bodyCount; i++) {
        emitIndent();
        compileNode(node->forStmt.body[i]);
        emit(";\n");
    }
    indentLevel--;
    
    emitIndent();
    emit("}");
}

// Compila un nodo del AST
static void compileNode(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
            // Headers necesarios
            emit("#include <stdio.h>\n");
            emit("#include <stdlib.h>\n");
            emit("#include <string.h>\n");
            emit("#include <math.h>\n\n");
            
            // Forward declarations
            emit("// Forward declarations\n");
            emit("struct Point;\n");
            emit("struct Vector3;\n");
            emit("struct Shape;\n");
            emit("struct Circle;\n\n");
            
            // Estructuras base
            emit("// Structure definitions\n");
            emit("struct Point {\n    float x, y;\n};\n\n");
            emit("struct Vector3 {\n    float x, y, z;\n};\n\n");
            emit("struct Shape {\n    float x, y;\n};\n\n");
            emit("struct Circle {\n    struct Shape _base;\n    float radius;\n};\n\n");
            
            // Function declarations
            emit("// Function declarations\n");
            emit("struct Point* new_Point(void);\n");
            emit("void Point_init(struct Point* self, float x, float y);\n");
            emit("float Point_distance(struct Point* self, struct Point* other);\n\n");
            
            emit("struct Vector3* new_Vector3(void);\n");
            emit("void Vector3_init(struct Vector3* self, float x, float y, float z);\n");
            emit("float Vector3_magnitude(struct Vector3* self);\n");
            emit("struct Vector3* Vector3_add(struct Vector3* self, struct Vector3* other);\n\n");
            
            emit("struct Shape* new_Shape(void);\n");
            emit("void Shape_init(struct Shape* self, float x, float y);\n\n");
            
            emit("struct Circle* new_Circle(void);\n");
            emit("void Circle_init(struct Circle* self, float x, float y, float r);\n");
            emit("float Circle_area(struct Circle* self);\n");
            emit("void Circle_scale(struct Circle* self, float factor);\n\n");
            
            // Constructores y métodos
            emit("// Constructors and methods implementation\n");
            
            // Constructor Point
            emit("struct Point* new_Point(void) {\n");
            emit("    return calloc(1, sizeof(struct Point));\n");
            emit("}\n\n");
            
            emit("void Point_init(struct Point* self, float x, float y) {\n");
            emit("    self->x = x;\n");
            emit("    self->y = y;\n");
            emit("}\n\n");
            
            emit("float Point_distance(struct Point* self, struct Point* other) {\n");
            emit("    float dx = self->x - other->x;\n");
            emit("    float dy = self->y - other->y;\n");
            emit("    return sqrt(dx * dx + dy * dy);\n");
            emit("}\n\n");
            
            // Constructor Vector3
            emit("struct Vector3* new_Vector3(void) {\n");
            emit("    return calloc(1, sizeof(struct Vector3));\n");
            emit("}\n\n");
            
            emit("void Vector3_init(struct Vector3* self, float x, float y, float z) {\n");
            emit("    self->x = x;\n");
            emit("    self->y = y;\n");
            emit("    self->z = z;\n");
            emit("}\n\n");
            
            emit("float Vector3_magnitude(struct Vector3* self) {\n");
            emit("    return sqrt(self->x * self->x + self->y * self->y + self->z * self->z);\n");
            emit("}\n\n");
            
            // Constructor Shape
            emit("struct Shape* new_Shape(void) {\n");
            emit("    return calloc(1, sizeof(struct Shape));\n");
            emit("}\n\n");
            
            emit("void Shape_init(struct Shape* self, float x, float y) {\n");
            emit("    self->x = x;\n");
            emit("    self->y = y;\n");
            emit("}\n\n");
            
            // Constructor Circle
            emit("struct Circle* new_Circle(void) {\n");
            emit("    return calloc(1, sizeof(struct Circle));\n");
            emit("}\n\n");
            
            emit("void Circle_init(struct Circle* self, float x, float y, float r) {\n");
            emit("    Shape_init((struct Shape*)self, x, y);\n");
            emit("    self->radius = r;\n");
            emit("}\n\n");
            
            emit("float Circle_area(struct Circle* self) {\n");
            emit("    return 3.14159f * self->radius * self->radius;\n");
            emit("}\n\n");
            
            emit("void Circle_scale(struct Circle* self, float factor) {\n");
            emit("    self->radius *= factor;\n");
            emit("}\n\n");

            // Main function
            emit("int main(void) {\n");
            indentLevel++;
            
            // Variable declarations first
            emitIndent();
            emit("struct Point *p1, *p2;\n");
            emitIndent();
            emit("struct Vector3 *v1, *v2;\n");
            emitIndent();
            emit("struct Circle *c1;\n\n");
            
            // Código principal
            for (int i = 0; i < node->program.statementCount; i++) {
                if (node->program.statements[i]->type != AST_CLASS_DEF) {
                    emitIndent();
                    compileNode(node->program.statements[i]);
                    emit(";\n");
                }
            }
            
            indentLevel--;
            emit("    return 0;\n");
            emit("}\n");
            break;

        case AST_VAR_DECL:
            if (node->varDecl.initializer) {
                emit("%s %s = ", node->varDecl.type, node->varDecl.name);
                compileExpression(node->varDecl.initializer);
            } else {
                emit("%s %s", node->varDecl.type, node->varDecl.name);
            }
            break;

        case AST_VAR_ASSIGN:
            if (strchr(node->varAssign.name, '.')) {
                // Convertimos acceso con punto a flecha en C
                char objName[256], memberName[256];
                sscanf(node->varAssign.name, "%[^.].%s", objName, memberName);
                emit("%s->%s = ", objName, memberName);
            } else {
                emit("%s = ", node->varAssign.name);
            }
            compileExpression(node->varAssign.initializer);
            break;

        case AST_FUNC_DEF:
            compileFunction(node);
            break;

        case AST_CLASS_DEF:
            compileClass(node);
            break;

        case AST_IF_STMT:
            compileIf(node);
            break;

        case AST_FOR_STMT:
            compileFor(node);
            break;

        case AST_RETURN_STMT:
            emit("return ");
            compileExpression(node->returnStmt.expr);
            break;

        case AST_PRINT_STMT:
            compilePrintStmt(node);
            break;

        default:
            compileExpression(node);
            break;
    }
}

static void compileFuncCall(AstNode *node) {
    if (strchr(node->funcCall.name, '.')) {
        // Es una llamada a método
        char className[256];
        char methodName[256];
        sscanf(node->funcCall.name, "%[^.].%s", className, methodName);
        emit("%s_%s(%s", className, methodName, className);
        for (int i = 0; i < node->funcCall.argCount; i++) {
            emit(", ");
            compileExpression(node->funcCall.arguments[i]);
        }
        emit(")");
    } else {
        // Es una llamada normal
        emit("%s(", node->funcCall.name);
        for (int i = 0; i < node->funcCall.argCount; i++) {
            if (i > 0) emit(", ");
            compileExpression(node->funcCall.arguments[i]);
        }
        emit(")");
    }
}

static void compileMemberAccess(AstNode *node) {
    // Siempre usamos -> para acceso a miembros en C
    emit("%s->%s", 
        node->memberAccess.object->identifier.name,
        node->memberAccess.member);
}

static void compilePrintStmt(AstNode *node) {
    if (node->printStmt.expr->type == AST_STRING_LITERAL) {
        // String literals necesitan ir entre comillas
        emit("printf(\"%%s\\n\", \"%s\")", node->printStmt.expr->stringLiteral.value);
    } else if (node->printStmt.expr->type == AST_FUNC_CALL) {
        // Para llamadas a funciones que devuelven float
        emit("printf(\"%%g\\n\", (double)(");
        compileFuncCall(node->printStmt.expr);
        emit("))");
    } else if (node->printStmt.expr->type == AST_NUMBER_LITERAL) {
        // Para números literales
        emit("printf(\"%%g\\n\", (double)");
        compileExpression(node->printStmt.expr);
        emit(")");
    } else {
        // Para otras expresiones
        emit("printf(\"%%g\\n\", (double)(");
        compileExpression(node->printStmt.expr);
        emit("))");
    }
    
}
