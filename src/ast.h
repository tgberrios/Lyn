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
    AST_IMPORT,           // Nodo para importaciones
    // New AST node types for control structures
    AST_DO_WHILE_STMT,    // Do-while statement
    AST_SWITCH_STMT,      // Switch statement
    AST_CASE_STMT,        // Case statement
    AST_TRY_CATCH_STMT,   // Try-catch statement
    AST_THROW_STMT,       // Throw statement
    AST_BREAK_STMT,       // Break statement
    AST_CURRY_EXPR,       // Curried function expression
    AST_PATTERN_MATCH,    // Pattern matching expression
    AST_PATTERN_CASE,     // Individual case in pattern matching
    AST_FUNC_COMPOSE,     // Function composition (f >> g)
    AST_MACRO_DEF,        // Macro definition
    AST_MACRO_EXPAND,     // Macro expansion
    AST_MACRO_PARAM,      // Macro parameter reference
    AST_ASPECT_DEF,       // Aspect definition
    AST_POINTCUT,         // Pointcut declaration
    AST_ADVICE,           // Advice declaration
    AST_BEFORE,           // Add if not exists
    AST_AFTER,            // Add if not exists
    AST_AROUND,           // Add if not exists
    AST_ARRAY_ACCESS,     // Nodo para acceso a arreglos
    AST_BOOLEAN_LITERAL,  // Nodo para literal booleano
    AST_UNARY_OP          // Nodo para operación unaria
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
            struct AstNode** body;  // Change from single body to array of statements
            int bodyCount;          // Add bodyCount field to match other statement types
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
        
        // Do-while statement
        struct {
            struct AstNode* condition;
            struct AstNode** body;  // List of statements in the do-while body
            int bodyCount;
        } doWhileStmt;
        
        // Switch statement
        struct {
            struct AstNode* expr;  // Expression to switch on
            struct AstNode** cases;  // List of case statements
            int caseCount;
            struct AstNode** defaultCase;  // Default case statements
            int defaultCaseCount;
        } switchStmt;
        
        // Case statement
        struct {
            struct AstNode* expr;  // Case value expression
            struct AstNode** body;  // List of statements in the case
            int bodyCount;
        } caseStmt;
        
        // Try-catch statement
        struct {
            struct AstNode** tryBody;  // List of statements in the try block
            int tryCount;
            struct AstNode** catchBody;  // List of statements in the catch block
            int catchCount;
            char errorVarName[256];  // Name of the error variable
            struct AstNode** finallyBody;  // List of statements in the finally block
            int finallyCount;
        } tryCatchStmt;
        
        // Throw statement
        struct {
            struct AstNode* expr;  // Expression to throw
        } throwStmt;
        
        // Break statement
        struct {
            // No additional fields needed
        } breakStmt;
        
        // Curried function expression
        struct {
            struct AstNode* baseFunc;     // Base function to curry
            struct AstNode** appliedArgs; // Arguments already applied
            int appliedCount;             // Number of applied arguments
            int totalArgCount;            // Total arguments expected
        } curryExpr;
        
        // Pattern matching expression
        struct {
            struct AstNode* expr;       // Expression to match against
            struct AstNode** cases;     // List of pattern cases
            int caseCount;
            struct AstNode* otherwise;  // Default case (optional)
        } patternMatch;
        
        // Pattern case
        struct {
            struct AstNode* pattern;    // Pattern to match
            struct AstNode** body;      // Body to execute if pattern matches
            int bodyCount;
        } patternCase;
        
        // Function composition
        struct {
            struct AstNode* left;      // Left function (executed first)
            struct AstNode* right;     // Right function (executed second)
        } funcCompose;
        
        // Macro definition
        struct {
            char name[256];
            char** params;         // Parameter names
            int paramCount;
            struct AstNode** body; // Macro body statements
            int bodyCount;
        } macroDef;
        
        // Macro expansion
        struct {
            char name[256];
            struct AstNode** args; // Arguments for expansion
            int argCount;
        } macroExpand;
        
        // Macro parameter reference
        struct {
            char name[256];
            int index;  // Parameter index in macro definition
        } macroParam;
        
        // Aspect definition
        struct {
            char name[256];
            struct AstNode** pointcuts;  // Lista de pointcuts
            int pointcutCount;
            struct AstNode** advice;     // Lista de advice
            int adviceCount;
        } aspectDef;
        
        // Pointcut declaration
        struct {
            char name[256];
            char pattern[512];           // Patrón de coincidencia (e.g., "*.onCreate()")
            int type;                    // Tipo de pointcut (método, constructor, etc.)
        } pointcut;
        
        // Advice declaration
        struct {
            int type;                    // BEFORE, AFTER, o AROUND
            char pointcutName[256];      // Nombre del pointcut al que se aplica
            struct AstNode** body;       // Código del advice
            int bodyCount;
        } advice;
        
        // Nodo para acceso a arreglos: array[index]
        struct {
            struct AstNode *array;  // Expresión del arreglo
            struct AstNode *index;  // Expresión del índice
        } arrayAccess;
        
        // Nodo para literal booleano
        struct {
            bool value;  // true o false
        } boolLiteral;
        
        // Nodo para operación unaria
        struct {
            char op;             // Operador ('N' para not)
            struct AstNode *expr;  // Expresión
        } unaryOp;
        
    };
} AstNode;

AstNode* createAstNode(AstNodeType type);
void freeAstNode(AstNode* node);
void freeAst(AstNode* root);

#endif
