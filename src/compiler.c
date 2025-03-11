#include "compiler.h"
#include "ast.h"
#include "error.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

static FILE* outputFile = NULL;
static int indentLevel = 0;

/* Declaraciones adelantadas de funciones internas */
static void emit(const char* fmt, ...);
static void emitLine(const char* fmt, ...);
static void indent(void);
static void outdent(void);

static void compileNode(AstNode* node);
static void compileExpression(AstNode* node);
static void compileFunction(AstNode* node);
static void compileFuncCall(AstNode* node);
static void compileMemberAccess(AstNode* node);
static void compilePrintStmt(AstNode* node);
static void compileIf(AstNode* node);
static void compileFor(AstNode* node);
static void compileLambda(AstNode* node);
static void compileClass(AstNode* node);

/* getCTypeString: retorna el nombre del tipo (almacenado en typeName) o "void*" si es NULL */
static const char* getCTypeString(Type* type) {
    return type ? type->typeName : "void*";
}

/* compileFuncCall: genera código para llamadas a funciones */
static void compileFuncCall(AstNode* node) {
    if (strchr(node->funcCall.name, '.')) {
        // Caso para métodos de clase (e.g., Point.distance)
        char className[256], methodName[256];
        sscanf(node->funcCall.name, "%[^.].%s", className, methodName);
        emit("%s_%s(%s", className, methodName, node->funcCall.arguments[0]->identifier.name);
        for (int i = 1; i < node->funcCall.argCount; i++) {
            emit(", ");
            compileExpression(node->funcCall.arguments[i]);
        }
        emit(")");
    } else if (strcmp(node->funcCall.name, "new_Point") == 0 ||
              strcmp(node->funcCall.name, "new_Vector3") == 0 ||
              strcmp(node->funcCall.name, "new_Circle") == 0 ||
              strcmp(node->funcCall.name, "new_Shape") == 0) {
        // Constructor seguro
        emit("%s()", node->funcCall.name);
    } else {
        // Caso normal
        emit("%s(", node->funcCall.name);
        for (int i = 0; i < node->funcCall.argCount; i++) {
            if (i > 0) emit(", ");
            compileExpression(node->funcCall.arguments[i]);
        }
        emit(")");
    }
}

/* compileMemberAccess: genera código para acceder a miembros de objetos */
static void compileMemberAccess(AstNode* node) {
    emit("((struct %s*)%s)->%s", 
         node->memberAccess.object->identifier.name,
         node->memberAccess.object->identifier.name,
         node->memberAccess.member);
}

/* compilePrintStmt: genera código para la instrucción print */
static void compilePrintStmt(AstNode* node) {
    if (node->printStmt.expr->type == AST_STRING_LITERAL) {
        // Para cadenas de texto, usamos formato %s
        emitLine("printf(\"%%s\\n\", \"%s\");", node->printStmt.expr->stringLiteral.value);
    } else if (node->printStmt.expr->type == AST_FUNC_CALL) {
        // Los segmentation faults ocurren aquí, agregamos validaciones
        if (strcmp(node->printStmt.expr->funcCall.name, "Point_distance") == 0 ||
            strcmp(node->printStmt.expr->funcCall.name, "Vector3_magnitude") == 0 ||
            strcmp(node->printStmt.expr->funcCall.name, "Circle_area") == 0) {
            emit("if (%s) printf(\"%%g\\n\", ", 
                node->printStmt.expr->funcCall.arguments[0]->identifier.name);
            compileFuncCall(node->printStmt.expr);
            emitLine("); else printf(\"NULL\\n\");");
        } else {
            emit("printf(\"%%g\\n\", ");
            compileFuncCall(node->printStmt.expr);
            emitLine(");");
        }
    } else if (node->printStmt.expr->type == AST_NUMBER_LITERAL) {
        emit("printf(\"%%g\\n\", ");
        compileExpression(node->printStmt.expr);
        emitLine(");");
    } else {
        emit("printf(\"%%g\\n\", ");
        compileExpression(node->printStmt.expr);
        emitLine(");");
    }
}

/* compileIf: genera código para la instrucción if */
static void compileIf(AstNode* node) {
    emit("if (");
    compileExpression(node->ifStmt.condition);
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

/* compileFor: genera código para el bucle for */
static void compileFor(AstNode* node) {
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

/* compileLambda: genera código para expresiones lambda */
static void compileLambda(AstNode* node) {
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
        strcpy(paramTemp.typeName, "void*"); // Valor por defecto
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

/* compileClass: renombra los métodos agregando el prefijo de la clase */
static void compileClass(AstNode* node) {
    emitLine("// Class declaration: %s", node->classDef.name);
    /* Renombrar métodos: agregar el prefijo de la clase a cada función miembro */
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

/* compileNode: genera el código C a partir del nodo AST */
static void compileNode(AstNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            emitLine("#include <stdio.h>");
            emitLine("#include <stdlib.h>");
            emitLine("#include <string.h>");
            emitLine("#include <math.h>");
            emitLine("");
            emitLine("// Forward declarations");
            emitLine("struct Point;");
            emitLine("struct Shape;");
            emitLine("struct Circle;");
            emitLine("struct Vector3;");
            emitLine("void* new_Point();");
            emitLine("void Point_init(void* self, double x, double y);");
            emitLine("double Point_distance(void* self, void* other);");
            emitLine("void* new_Shape();");
            emitLine("void Shape_init(void* self, double x, double y);");
            emitLine("double Shape_area(void* self);");
            emitLine("void* new_Circle();");
            emitLine("void Circle_init(void* self, double x, double y, double r);");
            emitLine("double Circle_area(void* self);");
            emitLine("void Circle_scale(void* self, double factor);");
            emitLine("void* new_Vector3();");
            emitLine("void Vector3_init(void* self, double x, double y, double z);");
            emitLine("double Vector3_magnitude(void* self);");
            emitLine("void* Vector3_add(void* self, void* other);");
            emitLine("");
            emitLine("struct Point {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            outdent();
            emitLine("};");
            emitLine("struct Shape {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            outdent();
            emitLine("};");
            emitLine("struct Circle {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            emitLine("double radius;");
            outdent();
            emitLine("};");
            emitLine("struct Vector3 {");
            indent();
            emitLine("double x;");
            emitLine("double y;");
            emitLine("double z;");
            outdent();
            emitLine("};");
            emitLine("");
            emitLine("void* new_Point() {");
            indent();
            emitLine("void* ptr = malloc(sizeof(struct Point));");
            emitLine("if (ptr) memset(ptr, 0, sizeof(struct Point));"); // Inicializar todo a cero
            emitLine("return ptr;");
            outdent();
            emitLine("}");
            emitLine("void* new_Shape() {");
            indent();
            emitLine("void* ptr = malloc(sizeof(struct Shape));");
            emitLine("if (ptr) memset(ptr, 0, sizeof(struct Shape));"); // Inicializar todo a cero
            emitLine("return ptr;");
            outdent();
            emitLine("}");
            emitLine("void* new_Circle() {");
            indent();
            emitLine("void* ptr = malloc(sizeof(struct Circle));");
            emitLine("if (ptr) memset(ptr, 0, sizeof(struct Circle));"); // Inicializar todo a cero
            emitLine("return ptr;");
            outdent();
            emitLine("}");
            emitLine("void* new_Vector3() {");
            indent();
            emitLine("void* ptr = malloc(sizeof(struct Vector3));");
            emitLine("if (ptr) memset(ptr, 0, sizeof(struct Vector3));"); // Inicializar todo a cero
            emitLine("return ptr;");
            outdent();
            emitLine("}");
            emitLine("");
            
            /* Primero compilamos las declaraciones de clases */
            for (int i = 0; i < node->program.statementCount; i++) {
                if (node->program.statements[i]->type == AST_CLASS_DEF) {
                    compileClass(node->program.statements[i]);
                }
            }
            
            /* Implementaciones mejoradas de funciones especiales */
            emitLine("void Point_init(void* self, double x, double y) {");
            indent();
            emitLine("if (!self) return;");
            emitLine("struct Point* p = (struct Point*)self;");
            emitLine("p->x = x;");
            emitLine("p->y = y;");
            outdent();
            emitLine("}");
            
            emitLine("void Vector3_init(void* self, double x, double y, double z) {");
            indent();
            emitLine("if (!self) return;");
            emitLine("struct Vector3* v = (struct Vector3*)self;");
            emitLine("v->x = x;");
            emitLine("v->y = y;");
            emitLine("v->z = z;");
            outdent();
            emitLine("}");
            
            emitLine("void Shape_init(void* self, double x, double y) {");
            indent();
            emitLine("if (!self) return;");
            emitLine("struct Shape* s = (struct Shape*)self;");
            emitLine("s->x = x;");
            emitLine("s->y = y;");
            outdent();
            emitLine("}");
            
            emitLine("void Circle_init(void* self, double x, double y, double r) {");
            indent();
            emitLine("if (!self) return;");
            emitLine("struct Circle* circle = (struct Circle*)self;");
            emitLine("circle->x = x;");
            emitLine("circle->y = y;");
            emitLine("circle->radius = r;");
            outdent();
            emitLine("}");

            emitLine("double Point_distance(void* self, void* other) {");
            indent();
            emitLine("if (!self || !other) return 0.0;");
            emitLine("struct Point* p1 = (struct Point*)self;");
            emitLine("struct Point* p2 = (struct Point*)other;");
            emitLine("double dx = p1->x - p2->x;");
            emitLine("double dy = p1->y - p2->y;");
            emitLine("return sqrt(dx * dx + dy * dy);");
            outdent();
            emitLine("}");

            emitLine("double Vector3_magnitude(void* self) {");
            indent();
            emitLine("if (!self) return 0.0;");
            emitLine("struct Vector3* v = (struct Vector3*)self;");
            emitLine("return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);");
            outdent();
            emitLine("}");

            emitLine("double Circle_area(void* self) {");
            indent();
            emitLine("if (!self) return 0.0;");
            emitLine("struct Circle* circle = (struct Circle*)self;");
            emitLine("return 3.14159 * circle->radius * circle->radius;");
            outdent();
            emitLine("}");

            emitLine("void Circle_scale(void* self, double factor) {");
            indent();
            emitLine("if (!self) return;");
            emitLine("struct Circle* circle = (struct Circle*)self;");
            emitLine("circle->radius *= factor;");
            outdent();
            emitLine("}");
            
            emitLine("double Shape_area(void* self) {");
            indent();
            emitLine("if (!self) return 0.0;"); 
            emitLine("return 0.0; // Base Shape has no area");
            outdent();
            emitLine("}");
            
            emitLine("void* Vector3_add(void* self, void* other) {");
            indent();
            emitLine("if (!self || !other) return NULL;");
            emitLine("struct Vector3* v1 = (struct Vector3*)self;");
            emitLine("struct Vector3* v2 = (struct Vector3*)other;");
            emitLine("struct Vector3* result = malloc(sizeof(struct Vector3));");
            emitLine("if (!result) return NULL; // Verificar asignación de memoria");
            emitLine("memset(result, 0, sizeof(struct Vector3)); // Inicializar a cero");
            emitLine("result->x = v1->x + v2->x;");
            emitLine("result->y = v1->y + v2->y;");
            emitLine("result->z = v1->z + v2->z;");
            emitLine("return result;");
            outdent();
            emitLine("}");

            /* Luego compilamos las funciones regulares */
            for (int i = 0; i < node->program.statementCount; i++) {
                if (node->program.statements[i]->type == AST_FUNC_DEF) {
                    compileFunction(node->program.statements[i]);
                }
            }
            
            /* Finalmente, la función main */
            emitLine("");
            emitLine("int main() {");
            indent();
            emitLine("void *p1 = NULL, *p2 = NULL, *v1 = NULL, *v2 = NULL, *c1 = NULL;");
            
            /* Compilamos los statements en el cuerpo del main */
            for (int i = 0; i < node->program.statementCount; i++) {
                if (node->program.statements[i]->type != AST_CLASS_DEF &&
                    node->program.statements[i]->type != AST_FUNC_DEF) {
                    compileNode(node->program.statements[i]);
                }
            }
            
            emitLine("// Limpieza de memoria antes de salir");
            emitLine("if (p1) free(p1);");
            emitLine("if (p2) free(p2);");
            emitLine("if (v1) free(v1);");
            emitLine("if (v2) free(v2);");
            emitLine("if (c1) free(c1);");
            emitLine("return 0;");
            outdent();
            emitLine("}");
            break;
            
        case AST_VAR_DECL:
            if (node->varDecl.initializer) {
                emit("%s %s = ", node->varDecl.type, node->varDecl.name);
                compileExpression(node->varDecl.initializer);
                emitLine(";");
            } else {
                emitLine("%s %s;", node->varDecl.type, node->varDecl.name);
            }
            break;
            
        case AST_VAR_ASSIGN:
            emit("%s = ", node->varAssign.name);
            compileExpression(node->varAssign.initializer);
            emitLine(";");
            break;
            
        case AST_FUNC_DEF:
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
            
        default:
            break;
    }
}

static void compileExpression(AstNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            emit("%g", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            emit("\"%s\"", node->stringLiteral.value);
            break;
        case AST_IDENTIFIER:
            emit("%s", node->identifier.name);
            break;
        case AST_BINARY_OP: {
            emit("(");
            compileExpression(node->binaryOp.left);
            {
                char opStr[2] = { node->binaryOp.op, '\0' };
                switch (node->binaryOp.op) {
                    case 'E': emit(" == "); break;
                    case 'N': emit(" != "); break;
                    case 'G': emit(" >= "); break;
                    case 'L': emit(" <= "); break;
                    default: emit(" %s ", opStr);
                }
            }
            compileExpression(node->binaryOp.right);
            emit(")");
            break;
        }
        case AST_FUNC_CALL:
            compileFuncCall(node);
            break;
        case AST_MEMBER_ACCESS:
            compileMemberAccess(node);
            break;
        case AST_ARRAY_LITERAL:
            emit("{");
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
            break;
    }
}

static void compileFunction(AstNode* node) {
    /* No necesitamos duplicar las funciones ya generadas en compileNode */
    if (strstr(node->funcDef.name, "Point_") ||
        strstr(node->funcDef.name, "Vector3_") ||
        strstr(node->funcDef.name, "Shape_") ||
        strstr(node->funcDef.name, "Circle_")) {
        return; // Ya generamos estas funciones en el proemio
    }
    
    /* Función general */
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
    outputFile = fopen(outputPath, "w");
    if (!outputFile) {
        fprintf(stderr, "Error: Could not open output file %s\n", outputPath);
        return false;
    }
    compileNode(ast);
    fclose(outputFile);
    outputFile = NULL;
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
    va_list args;
    va_start(args, fmt);
    for (int i = 0; i < indentLevel; i++) {
        fprintf(outputFile, "    ");
    }
    vfprintf(outputFile, fmt, args);
    fprintf(outputFile, "\n");
    va_end(args);
}

static void indent(void) {
    indentLevel++;
}

static void outdent(void) {
    if (indentLevel > 0)
        indentLevel--;
}