#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "types.h"  // Para usar el tipo Type

// Enumerador de tipos de nodo AST adaptado a parser.c y compiler.c
typedef enum {
    AST_PROGRAM,
    AST_NUMBER_LITERAL,
    AST_STRING_LITERAL,
    AST_IDENTIFIER,
    AST_VAR_DECL,
    AST_VAR_ASSIGN,
    AST_FUNC_DEF,         // Definición de función
    AST_EXPR_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BINARY_OP,        // Expresión binaria
    AST_FUNC_CALL,
    AST_MEMBER_ACCESS,
    AST_PRINT_STMT,
    AST_CLASS_DEF,        // Definición de clase
    AST_LAMBDA,
    AST_ARRAY_LITERAL,
    AST_MODULE_DECL,
    AST_IMPORT            // Nodo para importaciones
} AstNodeType;

// Enumerador para operadores binarios (usando el carácter)
typedef enum {
    OP_PLUS = '+',
    OP_MINUS = '-',
    OP_MULTIPLY = '*',
    OP_DIVIDE = '/'
} BinaryOperator;

// Definición de la estructura AST usando una unión para cada variante
typedef struct AstNode {
    AstNodeType type;
    int line;
    Type* inferredType; // Opcional
    union {
        // Nodo programa: lista de sentencias o declaraciones
        struct {
            struct AstNode** statements;
            int statementCount;
        } program;
        
        // Número literal
        struct {
            double value;
        } numberLiteral;
        
        // Cadena literal
        struct {
            char value[256];
        } stringLiteral;
        
        // Identificador
        struct {
            char name[256];
        } identifier;
        
        // Declaración de variable
        struct {
            char name[256];
            char type[128];  // Tipo en forma de cadena
            struct AstNode* initializer;
        } varDecl;
        
        // Asignación de variable
        struct {
            char name[256];
            struct AstNode* initializer;
        } varAssign;
        
        // Definición de función
        struct {
            char name[256];
            struct AstNode** parameters;  // Lista de identificadores (AST_IDENTIFIER)
            int paramCount;
            char returnType[64];
            struct AstNode** body;        // Lista de sentencias
            int bodyCount;
        } funcDef;
        
        // Sentencia de expresión
        struct {
            struct AstNode* expr;
        } exprStmt;
        
        // Sentencia if
        struct {
            struct AstNode* condition;
            struct AstNode** thenBranch;  // Lista de sentencias en la rama then
            int thenCount;
            struct AstNode** elseBranch;  // Lista de sentencias en la rama else (opcional)
            int elseCount;
        } ifStmt;
        
        // Sentencia while
        struct {
            struct AstNode* condition;
            struct AstNode* body;
        } whileStmt;
        
        // Sentencia for
        struct {
            char iterator[256];
            struct AstNode* rangeStart;
            struct AstNode* rangeEnd;
            struct AstNode** body;  // Lista de sentencias dentro del for
            int bodyCount;
        } forStmt;
        
        // Sentencia return
        struct {
            struct AstNode* expr;
        } returnStmt;
        
        // Expresión binaria
        struct {
            struct AstNode* left;
            char op; // Operador (por ejemplo, '+', '-', etc.)
            struct AstNode* right;
        } binaryOp;
        
        // Llamada a función
        struct {
            char name[256];
            struct AstNode** arguments;
            int argCount;
        } funcCall;
        
        // Acceso a miembro
        struct {
            struct AstNode* object;
            char member[256];
        } memberAccess;
        
        // Sentencia print
        struct {
            struct AstNode* expr;
        } printStmt;
        
        // Definición de clase
        struct {
            char name[256];
            char baseClassName[256];  // Nombre de la clase base (si existe)
            struct AstNode** members; // Lista de miembros (variables, funciones, etc.)
            int memberCount;
        } classDef;
        
        // Expresión lambda
        struct {
            struct AstNode** parameters;  // Lista de identificadores
            int paramCount;
            char returnType[64];
            struct AstNode* body;
        } lambda;
        
        // Literal de arreglo
        struct {
            struct AstNode** elements;
            int elementCount;
        } arrayLiteral;
        
        // Declaración de módulo
        struct {
            char name[256];
            struct AstNode** declarations;
            int declarationCount;
        } moduleDecl;
        
        // Importación de módulo
        struct {
            char moduleType[64];   // Por ejemplo: "ui" o "css"
            char moduleName[256];
        } importStmt;
    };
} AstNode;

AstNode* createAstNode(AstNodeType type);
void freeAstNode(AstNode* node);
void freeAst(AstNode* root);

#endif
