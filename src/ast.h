#ifndef AST_H
#define AST_H

#include <stddef.h>  // Para size_t
#include <stdbool.h> // Para bool

// Forward declarations
struct Type;

// Tipos de nodos AST
typedef enum {
    // Declaraciones de nivel superior
    AST_PROGRAM,
    AST_FUNC_DEF,
    AST_CLASS_DEF,
    AST_VAR_DECL,
    AST_IMPORT,
    AST_MODULE_DECL,
    AST_ASPECT_DEF,    // Se agregó para definir aspectos
    
    // Sentencias
    AST_BLOCK,
    AST_IF_STMT,
    AST_FOR_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_RETURN_STMT,
    AST_VAR_ASSIGN,
    AST_PRINT_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_TRY_CATCH_STMT,
    AST_THROW_STMT,
    
    // Expresiones
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_NUMBER_LITERAL,
    AST_STRING_LITERAL,
    AST_BOOLEAN_LITERAL,
    AST_NULL_LITERAL,
    AST_IDENTIFIER,
    AST_MEMBER_ACCESS,
    AST_ARRAY_ACCESS,
    AST_ARRAY_LITERAL,
    AST_FUNC_CALL,
    AST_LAMBDA,
    AST_FUNC_COMPOSE,
    AST_CURRY_EXPR,
    AST_NEW_EXPR,     // Nodo para instanciación de objetos (new)
    AST_THIS_EXPR,    // Nodo para la palabra clave "this"
    
    // Para programación orientada a aspectos
    AST_POINTCUT,
    AST_ADVICE,
    
    // Para pattern matching
    AST_PATTERN_MATCH,
    AST_PATTERN_CASE
} AstNodeType;

// Tipos de advice para programación orientada a aspectos
typedef enum {
    ADVICE_BEFORE = 0,
    ADVICE_AFTER  = 1,
    ADVICE_AROUND = 2
} AdviceType;

// Estructura base para todos los nodos AST
typedef struct AstNode {
    AstNodeType type;
    int line;  // Línea donde inicia el nodo
    int col;   // Columna donde inicia el nodo
    struct Type* inferredType;  // Para anotaciones de tipo inferido
    
    union {
        // AST_PROGRAM
        struct {
            struct AstNode** statements;
            int statementCount;
        } program;
        
        // AST_FUNC_DEF
        struct {
            char name[256];
            char returnType[64];
            struct AstNode** parameters;
            int paramCount;
            struct AstNode** body;
            int bodyCount;
        } funcDef;
        
        // AST_CLASS_DEF
        struct {
            char name[256];
            char baseClassName[256];
            struct AstNode** members;
            int memberCount;
        } classDef;
        
        // AST_VAR_DECL
        struct {
            char name[256];
            char type[64];
            struct AstNode* initializer;
        } varDecl;
        
        // AST_IMPORT
        struct {
            char moduleType[64];
            char moduleName[256];
        } importStmt;
        
        // AST_MODULE_DECL
        struct {
            char name[256];
            struct AstNode** declarations;
            int declarationCount;
        } moduleDecl;
        
        // AST_ASPECT_DEF
        struct {
            char name[256];
            struct AstNode** pointcuts;
            int pointcutCount;
            struct AstNode** advice;
            int adviceCount;
        } aspectDef;
        
        // AST_BLOCK
        struct {
            struct AstNode** statements;
            int statementCount;
        } block;
        
        // AST_IF_STMT
        struct {
            struct AstNode* condition;
            struct AstNode** thenBranch;
            int thenCount;
            struct AstNode** elseBranch;
            int elseCount;
        } ifStmt;
        
        // AST_FOR_STMT
        struct {
            char iterator[256];
            struct AstNode* rangeStart;
            struct AstNode* rangeEnd;
            struct AstNode** body;
            int bodyCount;
        } forStmt;
        
        // AST_WHILE_STMT
        struct {
            struct AstNode* condition;
            struct AstNode** body;
            int bodyCount;
        } whileStmt;
        
        // AST_DO_WHILE_STMT
        struct {
            struct AstNode* condition;
            struct AstNode** body;
            int bodyCount;
        } doWhileStmt;
        
        // AST_SWITCH_STMT
        struct {
            struct AstNode* expr;
            struct AstNode** cases;
            int caseCount;
            struct AstNode** defaultCase;
            int defaultCaseCount;
        } switchStmt;
        
        // AST_CASE_STMT
        struct {
            struct AstNode* expr;
            struct AstNode** body;
            int bodyCount;
        } caseStmt;
        
        // AST_RETURN_STMT
        struct {
            struct AstNode* expr;
        } returnStmt;
        
        // AST_VAR_ASSIGN
        struct {
            char name[256];
            struct AstNode* initializer;
        } varAssign;
        
        // AST_PRINT_STMT
        struct {
            struct AstNode* expr;
        } printStmt;
        
        // AST_BREAK_STMT
        struct {
            int dummy;  // C no permite struct vacía
        } breakStmt;
        
        // AST_CONTINUE_STMT
        struct {
            int dummy;  // C no permite struct vacía
        } continueStmt;
        
        // AST_TRY_CATCH_STMT
        struct {
            struct AstNode** tryBody;
            int tryCount;
            struct AstNode** catchBody;
            int catchCount;
            char errorVarName[256];
            struct AstNode** finallyBody;
            int finallyCount;
        } tryCatchStmt;
        
        // AST_THROW_STMT
        struct {
            struct AstNode* expr;
        } throwStmt;
        
        // AST_BINARY_OP
        struct {
            struct AstNode* left;
            char op;
            struct AstNode* right;
        } binaryOp;
        
        // AST_UNARY_OP
        struct {
            char op;
            struct AstNode* expr;
        } unaryOp;
        
        // AST_NUMBER_LITERAL
        struct {
            double value;
        } numberLiteral;
        
        // AST_STRING_LITERAL
        struct {
            char value[1024];
        } stringLiteral;
        
        // AST_BOOLEAN_LITERAL
        struct {
            bool value;
        } boolLiteral;
        
        // AST_NULL_LITERAL
        struct {
            int dummy;  // C no permite struct vacía
        } nullLiteral;
        
        // AST_IDENTIFIER
        struct {
            char name[256];
        } identifier;
        
        // AST_MEMBER_ACCESS
        struct {
            struct AstNode* object;
            char member[256];
        } memberAccess;
        
        // AST_ARRAY_ACCESS
        struct {
            struct AstNode* array;
            struct AstNode* index;
        } arrayAccess;
        
        // AST_ARRAY_LITERAL
        struct {
            struct AstNode** elements;
            int elementCount;
        } arrayLiteral;
        
        // AST_FUNC_CALL
        struct {
            char name[256];
            struct AstNode** arguments;
            int argCount;
        } funcCall;
        
        // AST_LAMBDA
        struct {
            struct AstNode** parameters;
            int paramCount;
            char returnType[64];
            struct AstNode* body;
        } lambda;
        
        // AST_FUNC_COMPOSE
        struct {
            struct AstNode* left;
            struct AstNode* right;
        } funcCompose;
        
        // AST_CURRY_EXPR
        struct {
            struct AstNode* baseFunc;
            struct AstNode** appliedArgs;
            int appliedCount;
            int totalArgCount;
        } curryExpr;
        
        // AST_NEW_EXPR (nueva: instanciación de objetos)
        struct {
            char className[256];
            struct AstNode** arguments;
            int argCount;
        } newExpr;
        
        // AST_THIS_EXPR (nueva: referencia a la instancia actual)
        struct {
            // No se requieren campos adicionales para 'this'
        } thisExpr;
        
        // AST_POINTCUT
        struct {
            char name[256];
            char pattern[1024];
        } pointcut;
        
        // AST_ADVICE
        struct {
            AdviceType type;
            char pointcutName[256];
            struct AstNode** body;
            int bodyCount;
        } advice;
        
        // AST_PATTERN_MATCH
        struct {
            struct AstNode* expr;
            struct AstNode** cases;
            int caseCount;
            struct AstNode* otherwise;
        } patternMatch;
        
        // AST_PATTERN_CASE
        struct {
            struct AstNode* pattern;
            struct AstNode** body;
            int bodyCount;
        } patternCase;
    };
} AstNode;

/* Funciones para trabajar con el AST */

// Inicializa el sistema AST
void ast_init(void);

// Limpia y libera recursos del sistema AST
void ast_cleanup(void);

// Establece el nivel de depuración para el sistema AST
void ast_set_debug_level(int level);

// Obtiene el nivel de depuración actual
int ast_get_debug_level(void);

// Crea un nuevo nodo AST del tipo especificado
AstNode* createAstNode(AstNodeType type);

// Libera un nodo AST y todos sus hijos
void freeAstNode(AstNode* node);

// Libera un programa AST completo
void freeAstProgram(AstNode* program);

// Imprime un AST para depuración
void printAst(AstNode* node, int indent);

// Copia un nodo AST (y todos sus hijos)
AstNode* copyAstNode(AstNode* node);

// Devuelve un string con el tipo de nodo AST
const char* astNodeTypeToString(AstNodeType type);

// Devuelve el número de hijos que tiene un nodo
int astNodeChildCount(AstNode* node);

// Devuelve el nº hijo de un nodo (0-based)
AstNode* astNodeGetChild(AstNode* node, int index);

// Estadísticas de uso de AST
typedef struct {
    int nodes_created;
    int nodes_freed;
    int max_depth;
    size_t memory_used;
} AstStats;

// Obtiene estadísticas de uso de nodos AST
AstStats ast_get_stats(void);

#endif /* AST_H */
