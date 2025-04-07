#ifndef AST_H
#define AST_H

#include <stddef.h>  // For size_t
#include <stdbool.h> // For bool

/**
 * @file ast.h
 * @brief Header file for the Abstract Syntax Tree (AST) system
 * 
 * This header file defines the core structures and functions for managing
 * Abstract Syntax Trees in the Lyn compiler. It includes definitions for
 * all AST node types, their associated data structures, and the functions
 * for manipulating these trees.
 */

// Forward declarations
struct Type;

/**
 * @brief Enumeration of all possible AST node types
 * 
 * This enumeration defines all the different types of nodes that can appear
 * in the Abstract Syntax Tree. The types are organized into categories:
 * - Top-level declarations
 * - Statements
 * - Expressions
 * - Aspect-oriented programming constructs
 * - Pattern matching constructs
 */
typedef enum {
    // Top-level declarations
    AST_PROGRAM,
    AST_FUNC_DEF,
    AST_CLASS_DEF,
    AST_VAR_DECL,
    AST_IMPORT,
    AST_MODULE_DECL,
    AST_ASPECT_DEF,    // Added for aspect definitions
    
    // Statements
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
    
    // Expressions
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
    AST_NEW_EXPR,     // Node for object instantiation (new)
    AST_THIS_EXPR,    // Node for the "this" keyword
    
    // Aspect-oriented programming
    AST_POINTCUT,
    AST_ADVICE,
    
    // Pattern matching
    AST_PATTERN_MATCH,
    AST_PATTERN_CASE
} AstNodeType;

/**
 * @brief Types of advice in aspect-oriented programming
 * 
 * Defines the different types of advice that can be applied in aspect-oriented
 * programming: before, after, or around a join point.
 */
typedef enum {
    ADVICE_BEFORE = 0,  // Advice executed before the join point
    ADVICE_AFTER  = 1,  // Advice executed after the join point
    ADVICE_AROUND = 2   // Advice that can control the execution of the join point
} AdviceType;

/**
 * @brief Types of for loops supported by the language
 * 
 * Defines the different styles of for loops that can be used:
 * - Range-based iteration
 * - Collection-based iteration
 * - Traditional C-style iteration
 */
typedef enum {
    FOR_RANGE = 0,      // for i in range(start, end)
    FOR_COLLECTION = 1, // for elem in collection
    FOR_TRADITIONAL = 2 // for (init; condition; update)
} ForLoopType;

/**
 * @brief Base structure for all AST nodes
 * 
 * This structure represents a node in the Abstract Syntax Tree. It uses
 * a discriminated union to store the specific data for each type of node.
 * All nodes share common fields for type, location, and inferred type.
 */
typedef struct AstNode {
    AstNodeType type;           // Type of the AST node
    int line;                   // Line number where the node begins
    int col;                    // Column number where the node begins
    struct Type* inferredType;  // For type inference annotations
    
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
            char moduleType[64];             // Tipo de módulo (normal, ui, css)
            char moduleName[256];            // Nombre del módulo
            char alias[256];                 // Alias del módulo (si existe)
            bool hasAlias;                   // Indica si se usa un alias
            bool hasSymbolList;              // Si es una importación con lista de símbolos
            const char** symbols;            // Símbolos a importar
            const char** aliases;            // Alias para los símbolos
            int symbolCount;                 // Número de símbolos
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
            ForLoopType forType;    // Type of for loop
            char iterator[256];     // Iterator name (for range and collection)
            struct AstNode* rangeStart; // For range
            struct AstNode* rangeEnd;   // For range
            struct AstNode* rangeStep;  // Step for range (optional)
            struct AstNode* collection; // For collection iteration
            struct AstNode* init;       // For traditional loop
            struct AstNode* condition;  // For traditional loop
            struct AstNode* update;     // For traditional loop
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
            int dummy;  // C doesn't allow empty structs
        } breakStmt;
        
        // AST_CONTINUE_STMT
        struct {
            int dummy;  // C doesn't allow empty structs
        } continueStmt;
        
        // AST_TRY_CATCH_STMT
        struct {
            struct AstNode** tryBody;
            int tryCount;
            struct AstNode** catchBody;
            int catchCount;
            char errorVarName[256];
            char errorType[64];  // Added for error type checking
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
            int dummy;  // C doesn't allow empty structs
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
        
        // AST_NEW_EXPR (new: object instantiation)
        struct {
            char className[256];
            struct AstNode** arguments;
            int argCount;
        } newExpr;
        
        // AST_THIS_EXPR (new: reference to current instance)
        struct {
            // No additional fields required for 'this'
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

/**
 * @brief Statistics about AST usage
 * 
 * This structure keeps track of various metrics about AST usage,
 * including node creation, memory usage, and tree depth.
 */
typedef struct {
    int nodes_created;    // Number of nodes created
    int nodes_freed;      // Number of nodes freed
    int max_depth;        // Maximum depth of any AST tree
    size_t memory_used;   // Total memory used by AST nodes
} AstStats;

/* AST manipulation functions */

/**
 * @brief Initializes the AST system
 */
void ast_init(void);

/**
 * @brief Cleans up and frees resources from the AST system
 */
void ast_cleanup(void);

/**
 * @brief Sets the debug level for the AST system
 * 
 * @param level The new debug level (0=minimum, 3=maximum)
 */
void ast_set_debug_level(int level);

/**
 * @brief Gets the current debug level
 * 
 * @return int The current debug level
 */
int ast_get_debug_level(void);

/**
 * @brief Creates a new AST node of the specified type
 * 
 * @param type The type of AST node to create
 * @return AstNode* The newly created node, or NULL if allocation fails
 */
AstNode* createAstNode(AstNodeType type);

/**
 * @brief Frees an AST node and all its children
 * 
 * @param node The AST node to free
 */
void freeAstNode(AstNode* node);

/**
 * @brief Frees a complete AST program
 * 
 * @param program The program node to free
 */
void freeAstProgram(AstNode* program);

/**
 * @brief Prints an AST for debugging purposes
 * 
 * @param node The AST node to print
 * @param indent The current indentation level
 */
void printAst(AstNode* node, int indent);

/**
 * @brief Creates a copy of an AST node
 * 
 * @param node The AST node to copy
 * @return AstNode* A copy of the node, or NULL if allocation fails
 */
AstNode* copyAstNode(AstNode* node);

/**
 * @brief Converts an AST node type to its string representation
 * 
 * @param type The AST node type to convert
 * @return const char* String representation of the node type
 */
const char* astNodeTypeToString(AstNodeType type);

/**
 * @brief Gets the number of children a node has
 * 
 * @param node The AST node to check
 * @return int The number of children
 */
int astNodeChildCount(AstNode* node);

/**
 * @brief Gets a specific child of a node
 * 
 * @param node The parent AST node
 * @param index The index of the child to get (0-based)
 * @return AstNode* The requested child node, or NULL if index is invalid
 */
AstNode* astNodeGetChild(AstNode* node, int index);

/**
 * @brief Gets statistics about AST usage
 * 
 * @return AstStats Current statistics about AST usage
 */
AstStats ast_get_stats(void);

#endif /* AST_H */
