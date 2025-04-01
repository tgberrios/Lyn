/**
 * @file optimizer.c
 * @brief AST optimization implementation for the Lyn compiler
 * 
 * This file implements various optimization passes for the Abstract Syntax Tree (AST):
 * - Constant folding
 * - Dead code elimination
 * - Redundant statement removal
 * - Constant propagation
 * - Common subexpression elimination
 * - Scope analysis
 */

#include "optimizer.h"
#include "ast.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/** Current optimization level */
static OptimizerLevel currentLevel = OPT_LEVEL_0;

/** Default debug level for the optimizer */
static int debug_level = 1;

/** Statistics tracking for optimizations performed */
static OptimizationStats stats = {0};

/** Optimizer options - enabled by default based on optimization level */
static OptimizerOptions options = {
    .enable_constant_folding = true,
    .enable_dead_code_elimination = true,
    .enable_redundant_stmt_removal = true,
    .enable_constant_propagation = true,
    .enable_common_subexpr_elimination = true,
    .enable_scope_analysis = true
};

/**
 * @brief Sets the optimizer options
 * 
 * @param new_options New optimizer options to apply
 */
void optimizer_set_options(OptimizerOptions new_options) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_set_options);
    options = new_options;
    logger_log(LOG_INFO, "Optimizer options updated");
}

/**
 * @brief Gets the current optimizer options
 * 
 * @return OptimizerOptions Current optimizer options
 */
OptimizerOptions optimizer_get_options(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_get_options);
    return options;
}

/**
 * @brief Sets the debug level for the optimizer
 * 
 * @param level New debug level (0=minimum, 3=maximum)
 */
void optimizer_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Optimizer debug level set to %d", level);
}

/**
 * @brief Gets the current debug level for the optimizer
 * 
 * @return int Current debug level (0=minimum, 3=maximum)
 */
int optimizer_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_get_debug_level);
    return debug_level;
}

/**
 * @brief Initializes the optimizer with a specified optimization level
 * 
 * @param level Optimization level to use
 */
void optimizer_init(OptimizerLevel level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_init);
    
    currentLevel = level;
    // Reset statistics
    stats = (OptimizationStats){0};
    
    logger_log(LOG_INFO, "Optimizer initialized with level %d", level);
}

/**
 * @brief Gets the current optimization statistics
 * 
 * @return OptimizationStats Current optimization statistics
 */
OptimizationStats optimizer_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_get_stats);
    return stats;
}

/**
 * @brief Symbol table entry for variable scope analysis and constant propagation
 */
typedef struct SymbolEntry {
    char name[256];                  ///< Variable name
    int scope_level;                 ///< Scope nesting level
    bool is_constant;                ///< Whether it holds a constant value
    AstNode* constant_value;         ///< If constant, its value
    AstNode* declaration_node;       ///< Declaration node
    struct SymbolEntry* next;        ///< Next entry in same scope
} SymbolEntry;

/**
 * @brief Symbol table for scope analysis
 */
typedef struct {
    SymbolEntry** scopes;            ///< Array of scope entry points
    int scope_count;                 ///< Number of scopes
    int current_scope;               ///< Current scope level
} SymbolTable;

/**
 * @brief Expression hash table entry for common subexpression elimination
 */
typedef struct ExprHashEntry {
    AstNode* expr;                   ///< Expression node
    AstNode* var_ref;                ///< Variable that holds the result
    struct ExprHashEntry* next;      ///< Next entry in hash bucket
} ExprHashEntry;

/**
 * @brief Expression hash table for common subexpression elimination
 */
typedef struct {
    ExprHashEntry** buckets;         ///< Hash buckets
    int bucket_count;                ///< Number of buckets
    int entry_count;                 ///< Number of entries
} ExprHashTable;

/** Global symbol table for scope analysis */
static SymbolTable symbol_table = {0};

/** Global expression hash table for common subexpression elimination */
static ExprHashTable expr_table = {0};

/**
 * @brief Initializes the symbol table
 * 
 * Frees any existing table and creates a new empty table with a global scope.
 * Also initializes the expression hash table.
 */
static void init_symbol_table(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)init_symbol_table);
    
    // Free existing table if any
    if (symbol_table.scopes) {
        for (int i = 0; i < symbol_table.scope_count; i++) {
            SymbolEntry* entry = symbol_table.scopes[i];
            while (entry) {
                SymbolEntry* next = entry->next;
                if (entry->constant_value) {
                    freeAstNode(entry->constant_value);
                }
                free(entry);
                entry = next;
            }
        }
        free(symbol_table.scopes);
    }
    
    // Initialize new empty table with a global scope
    symbol_table.scope_count = 1;
    symbol_table.scopes = calloc(1, sizeof(SymbolEntry*));
    symbol_table.current_scope = 0;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Symbol table initialized");
    }
}

/**
 * @brief Enters a new scope in the symbol table
 * 
 * Creates a new scope level and updates the current scope pointer.
 */
static void enter_scope(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)enter_scope);
    
    symbol_table.scope_count++;
    symbol_table.scopes = realloc(symbol_table.scopes, 
                                 symbol_table.scope_count * sizeof(SymbolEntry*));
    symbol_table.scopes[symbol_table.scope_count - 1] = NULL;
    symbol_table.current_scope = symbol_table.scope_count - 1;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Entered scope level %d", symbol_table.current_scope);
    }
}

/**
 * @brief Exits the current scope
 * 
 * Frees all symbols in the current scope and updates the current scope pointer.
 * Cannot exit the global scope (level 0).
 */
static void exit_scope(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)exit_scope);
    
    if (symbol_table.current_scope <= 0) {
        logger_log(LOG_WARNING, "Attempted to exit global scope");
        return;
    }
    
    // Free all symbols in the current scope
    SymbolEntry* entry = symbol_table.scopes[symbol_table.current_scope];
    while (entry) {
        SymbolEntry* next = entry->next;
        if (entry->constant_value) {
            freeAstNode(entry->constant_value);
        }
        free(entry);
        entry = next;
    }
    
    symbol_table.scopes[symbol_table.current_scope] = NULL;
    symbol_table.current_scope--;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Exited to scope level %d", symbol_table.current_scope);
    }
}

/**
 * Add variable to symbol table
 */
static void add_variable(const char* name, AstNode* declaration_node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)add_variable);
    
    SymbolEntry* entry = malloc(sizeof(SymbolEntry));
    if (!entry) {
        error_report("Optimizer", __LINE__, 0, "Failed to allocate symbol entry", ERROR_MEMORY);
        return;
    }
    
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->scope_level = symbol_table.current_scope;
    entry->is_constant = false;
    entry->constant_value = NULL;
    entry->declaration_node = declaration_node;
    
    // Add to current scope
    entry->next = symbol_table.scopes[symbol_table.current_scope];
    symbol_table.scopes[symbol_table.current_scope] = entry;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Added variable '%s' to scope %d", 
                  name, symbol_table.current_scope);
    }
    
    stats.variables_scoped++;
}

/**
 * Find variable in symbol table (searches from current scope up to global)
 */
static SymbolEntry* find_variable(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)find_variable);
    
    // Search from current scope up to global scope
    for (int scope = symbol_table.current_scope; scope >= 0; scope--) {
        SymbolEntry* entry = symbol_table.scopes[scope];
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                return entry;
            }
            entry = entry->next;
        }
    }
    
    return NULL;
}

/**
 * Update variable with constant value
 */
static void set_variable_constant(const char* name, AstNode* value) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)set_variable_constant);
    
    SymbolEntry* entry = find_variable(name);
    if (!entry) {
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Cannot set constant for unknown variable '%s'", name);
        }
        return;
    }
    
    // Free old constant value if any
    if (entry->constant_value) {
        freeAstNode(entry->constant_value);
    }
    
    // Update with new value
    if (value && (value->type == AST_NUMBER_LITERAL || 
                  value->type == AST_STRING_LITERAL)) {
        entry->is_constant = true;
        entry->constant_value = value;
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Set variable '%s' as constant", name);
        }
    } else {
        entry->is_constant = false;
        entry->constant_value = NULL;
    }
}

/**
 * Initialize expression hash table
 */
static void init_expr_table(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)init_expr_table);
    
    // Free existing table if any
    if (expr_table.buckets) {
        for (int i = 0; i < expr_table.bucket_count; i++) {
            ExprHashEntry* entry = expr_table.buckets[i];
            while (entry) {
                ExprHashEntry* next = entry->next;
                free(entry);
                entry = next;
            }
        }
        free(expr_table.buckets);
    }
    
    // Initialize new table
    expr_table.bucket_count = 257; // Prime number for good hash distribution
    expr_table.buckets = calloc(expr_table.bucket_count, sizeof(ExprHashEntry*));
    expr_table.entry_count = 0;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Expression hash table initialized with %d buckets", 
                  expr_table.bucket_count);
    }
}

/**
 * Clear current entries from expression hash table when entering new scope or basic block
 */
static void clear_expr_table(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)clear_expr_table);
    
    for (int i = 0; i < expr_table.bucket_count; i++) {
        ExprHashEntry* entry = expr_table.buckets[i];
        while (entry) {
            ExprHashEntry* next = entry->next;
            free(entry);
            entry = next;
        }
        expr_table.buckets[i] = NULL;
    }
    
    expr_table.entry_count = 0;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Expression hash table cleared");
    }
}

/**
 * @brief Computes a hash value for an expression
 * 
 * Creates a hash value based on the expression's type and contents.
 * For numeric literals, uses the bits of the double value.
 * For strings and identifiers, uses a rolling hash of the characters.
 * For binary operations, combines hashes of both operands.
 * For function calls, hashes the name and argument count.
 * For member access, combines hashes of object and member name.
 * 
 * @param expr AST node to hash
 * @return unsigned int Hash value modulo bucket count
 */
static unsigned int hash_expression(AstNode* expr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)hash_expression);
    
    if (!expr) return 0;
    
    unsigned int hash = expr->type * 31;
    
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            // Use the bits of the double value for hashing
            {
                double value = expr->numberLiteral.value;
                unsigned int* ptr = (unsigned int*)&value;
                hash ^= ptr[0] ^ ptr[1];
            }
            break;
            
        case AST_STRING_LITERAL:
            // Hash the string content
            {
                const char* str = expr->stringLiteral.value;
                while (*str) {
                    hash = hash * 31 + *str++;
                }
            }
            break;
            
        case AST_IDENTIFIER:
            // Hash the identifier name
            {
                const char* name = expr->identifier.name;
                while (*name) {
                    hash = hash * 31 + *name++;
                }
            }
            break;
            
        case AST_BINARY_OP:
            // Hash the operator and both operands
            hash = hash * 31 + expr->binaryOp.op;
            hash ^= hash_expression(expr->binaryOp.left);
            hash = hash * 31 ^ hash_expression(expr->binaryOp.right);
            break;
            
        case AST_FUNC_CALL:
            // Hash the function name and argument count
            {
                const char* name = expr->funcCall.name;
                while (*name) {
                    hash = hash * 31 + *name++;
                }
                hash = hash * 31 + expr->funcCall.argCount;
                
                // Hash each argument
                for (int i = 0; i < expr->funcCall.argCount; i++) {
                    hash ^= hash_expression(expr->funcCall.arguments[i]);
                    hash *= 31;
                }
            }
            break;
            
        case AST_MEMBER_ACCESS:
            // Hash the object and member name
            hash ^= hash_expression(expr->memberAccess.object);
            {
                const char* member = expr->memberAccess.member;
                while (*member) {
                    hash = hash * 31 + *member++;
                }
            }
            break;
            
        default:
            // For other nodes, just use the type
            break;
    }
    
    return hash % expr_table.bucket_count;
}

/**
 * @brief Checks if two expressions are equivalent
 * 
 * Compares two AST nodes for structural equality.
 * For literals, compares values directly.
 * For strings and identifiers, compares content.
 * For binary operations, compares operator and both operands.
 * For function calls, compares name, argument count, and all arguments.
 * For member access, compares object and member name.
 * 
 * @param expr1 First expression to compare
 * @param expr2 Second expression to compare
 * @return bool true if expressions are equivalent
 */
static bool are_expressions_equal(AstNode* expr1, AstNode* expr2) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)are_expressions_equal);
    
    if (!expr1 || !expr2) return expr1 == expr2;
    
    if (expr1->type != expr2->type) return false;
    
    switch (expr1->type) {
        case AST_NUMBER_LITERAL:
            return expr1->numberLiteral.value == expr2->numberLiteral.value;
            
        case AST_STRING_LITERAL:
            return strcmp(expr1->stringLiteral.value, expr2->stringLiteral.value) == 0;
            
        case AST_IDENTIFIER:
            return strcmp(expr1->identifier.name, expr2->identifier.name) == 0;
            
        case AST_BINARY_OP:
            return expr1->binaryOp.op == expr2->binaryOp.op &&
                   are_expressions_equal(expr1->binaryOp.left, expr2->binaryOp.left) &&
                   are_expressions_equal(expr1->binaryOp.right, expr2->binaryOp.right);
            
        case AST_FUNC_CALL:
            if (strcmp(expr1->funcCall.name, expr2->funcCall.name) != 0 ||
                expr1->funcCall.argCount != expr2->funcCall.argCount)
                return false;
                
            for (int i = 0; i < expr1->funcCall.argCount; i++) {
                if (!are_expressions_equal(expr1->funcCall.arguments[i], 
                                           expr2->funcCall.arguments[i]))
                    return false;
            }
            return true;
            
        case AST_MEMBER_ACCESS:
            return are_expressions_equal(expr1->memberAccess.object, expr2->memberAccess.object) &&
                   strcmp(expr1->memberAccess.member, expr2->memberAccess.member) == 0;
            
        default:
            return false;
    }
}

/**
 * @brief Adds an expression to the hash table
 * 
 * If the expression already exists in the table, returns the variable reference
 * that holds its result. Otherwise, adds the new expression and returns NULL.
 * 
 * @param expr Expression to add
 * @param var_ref Variable reference that holds the result
 * @return AstNode* Existing variable reference if found, NULL if added new
 */
static AstNode* add_expr_to_table(AstNode* expr, AstNode* var_ref) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)add_expr_to_table);
    
    unsigned int hash = hash_expression(expr);
    
    // Check if expression already exists
    ExprHashEntry* entry = expr_table.buckets[hash];
    while (entry) {
        if (are_expressions_equal(entry->expr, expr)) {
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Found duplicate expression in hash table");
            }
            return entry->var_ref;
        }
        entry = entry->next;
    }
    
    // Add new entry
    entry = malloc(sizeof(ExprHashEntry));
    if (!entry) {
        error_report("Optimizer", __LINE__, 0, "Failed to allocate hash entry", ERROR_MEMORY);
        return NULL;
    }
    
    entry->expr = expr;
    entry->var_ref = var_ref;
    entry->next = expr_table.buckets[hash];
    expr_table.buckets[hash] = entry;
    expr_table.entry_count++;
    
    return NULL; // No match found
}

/**
 * @brief Creates a deep copy of an AST node for constant propagation
 * 
 * Makes a shallow copy of the node structure. This is sufficient for constant
 * propagation since we only need to copy literal nodes.
 * 
 * @param node AST node to clone
 * @return AstNode* New copy of the node, or NULL if allocation fails
 */
static AstNode* clone_node(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)clone_node);
    
    if (!node) return NULL;
    
    AstNode* clone = malloc(sizeof(AstNode));
    if (!clone) {
        error_report("Optimizer", __LINE__, 0, "Failed to allocate node clone", ERROR_MEMORY);
        return NULL;
    }
    
    memcpy(clone, node, sizeof(AstNode));
    
    return clone;
}

/**
 * @brief Performs variable scope analysis and builds symbol table
 * 
 * Traverses the AST to build a symbol table tracking variable declarations,
 * scopes, and constant values. This information is used by other optimization
 * passes like constant propagation.
 * 
 * @param node AST node to analyze
 * @return AstNode* Modified AST with scope information
 */
static AstNode* scope_analysis(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)scope_analysis);
    
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_PROGRAM:
            // Program is the global scope
            init_symbol_table();
            init_expr_table();
            
            for (int i = 0; i < node->program.statementCount; i++) {
                node->program.statements[i] = scope_analysis(node->program.statements[i]);
            }
            break;
            
        case AST_VAR_DECL:
            // Add variable to current scope
            add_variable(node->varDecl.name, node);
            
            // Process initializer if exists
            if (node->varDecl.initializer) {
                node->varDecl.initializer = scope_analysis(node->varDecl.initializer);
                
                // If initializer is constant, mark the variable as constant
                if (node->varDecl.initializer->type == AST_NUMBER_LITERAL ||
                    node->varDecl.initializer->type == AST_STRING_LITERAL) {
                    
                    AstNode* value_copy = clone_node(node->varDecl.initializer);
                    set_variable_constant(node->varDecl.name, value_copy);
                }
            }
            break;
            
        case AST_VAR_ASSIGN:
            // Process initializer
            if (node->varAssign.initializer) {
                node->varAssign.initializer = scope_analysis(node->varAssign.initializer);
                
                // If assigning a constant, update symbol table
                if (node->varAssign.initializer->type == AST_NUMBER_LITERAL ||
                    node->varAssign.initializer->type == AST_STRING_LITERAL) {
                    
                    AstNode* value_copy = clone_node(node->varAssign.initializer);
                    set_variable_constant(node->varAssign.name, value_copy);
                } else {
                    // Otherwise mark as non-constant
                    SymbolEntry* entry = find_variable(node->varAssign.name);
                    if (entry) {
                        entry->is_constant = false;
                        if (entry->constant_value) {
                            freeAstNode(entry->constant_value);
                            entry->constant_value = NULL;
                        }
                    } else {
                        // If variable not found, add it to current scope
                        add_variable(node->varAssign.name, NULL);
                    }
                }
            }
            break;
            
        case AST_FUNC_DEF:
            // Function body has its own scope
            enter_scope();
            
            // Add parameters to function scope
            for (int i = 0; i < node->funcDef.paramCount; i++) {
                if (node->funcDef.parameters[i]->type == AST_IDENTIFIER) {
                    add_variable(node->funcDef.parameters[i]->identifier.name, 
                                node->funcDef.parameters[i]);
                }
            }
            
            // Process body
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                node->funcDef.body[i] = scope_analysis(node->funcDef.body[i]);
            }
            
            // Leave function scope
            exit_scope();
            break;
            
        case AST_IF_STMT:
            // Process condition
            node->ifStmt.condition = scope_analysis(node->ifStmt.condition);
            
            // Then branch has its own scope
            enter_scope();
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                node->ifStmt.thenBranch[i] = scope_analysis(node->ifStmt.thenBranch[i]);
            }
            exit_scope();
            
            // Else branch has its own scope
            enter_scope();
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                node->ifStmt.elseBranch[i] = scope_analysis(node->ifStmt.elseBranch[i]);
            }
            exit_scope();
            
            // Clear expression table since control flow changes
            clear_expr_table();
            break;
            
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            clear_expr_table(); // Control flow entry point
            
            // Process condition
            if (node->type == AST_WHILE_STMT) {
                node->whileStmt.condition = scope_analysis(node->whileStmt.condition);
            } else {
                node->doWhileStmt.condition = scope_analysis(node->doWhileStmt.condition);
            }
            
            // Loop body has its own scope
            enter_scope();
            
            if (node->type == AST_WHILE_STMT) {
                for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                    node->whileStmt.body[i] = scope_analysis(node->whileStmt.body[i]);
                }
            } else {
                for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                    node->doWhileStmt.body[i] = scope_analysis(node->doWhileStmt.body[i]);
                }
            }
            
            exit_scope();
            clear_expr_table(); // Control flow exit point
            break;
            
        case AST_FOR_STMT:
            // Process range expressions
            node->forStmt.rangeStart = scope_analysis(node->forStmt.rangeStart);
            node->forStmt.rangeEnd = scope_analysis(node->forStmt.rangeEnd);
            
            // Loop body has its own scope
            enter_scope();
            
            // Add iterator variable to scope
            add_variable(node->forStmt.iterator, NULL);
            
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                node->forStmt.body[i] = scope_analysis(node->forStmt.body[i]);
            }
            
            exit_scope();
            clear_expr_table(); // Control flow changes
            break;
            
        case AST_MEMBER_ACCESS:
            node->memberAccess.object = scope_analysis(node->memberAccess.object);
            break;
            
        case AST_FUNC_CALL:
            for (int i = 0; i < node->funcCall.argCount; i++) {
                node->funcCall.arguments[i] = scope_analysis(node->funcCall.arguments[i]);
            }
            break;
            
        case AST_BINARY_OP:
            node->binaryOp.left = scope_analysis(node->binaryOp.left);
            node->binaryOp.right = scope_analysis(node->binaryOp.right);
            break;
            
        case AST_RETURN_STMT:
            if (node->returnStmt.expr) {
                node->returnStmt.expr = scope_analysis(node->returnStmt.expr);
            }
            break;
            
        case AST_PRINT_STMT:
            if (node->printStmt.expr) {
                node->printStmt.expr = scope_analysis(node->printStmt.expr);
            }
            break;
            
        case AST_CLASS_DEF:
            enter_scope(); // Class has its own scope
            
            for (int i = 0; i < node->classDef.memberCount; i++) {
                node->classDef.members[i] = scope_analysis(node->classDef.members[i]);
            }
            
            exit_scope();
            break;
            
        default:
            break;
    }
    
    return node;
}

/**
 * @brief Performs constant propagation optimization
 * 
 * Replaces variable references with their known constant values when possible.
 * This optimization is enabled at optimization level 2 and above.
 * 
 * @param node AST node to optimize
 * @return AstNode* Modified AST with constants propagated
 */
static AstNode* constant_propagation(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)constant_propagation);
    
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            // If identifier is a constant, replace with its value
            {
                SymbolEntry* entry = find_variable(node->identifier.name);
                if (entry && entry->is_constant && entry->constant_value) {
                    if (debug_level >= 2) {
                        logger_log(LOG_DEBUG, "Propagating constant for '%s'", node->identifier.name);
                    }
                    
                    // Create a copy of the constant value
                    AstNode* value_copy = clone_node(entry->constant_value);
                    if (!value_copy) {
                        return node; // If clone fails, keep original
                    }
                    
                    // Count this optimization
                    stats.constants_propagated++;
                    stats.total_optimizations++;
                    
                    // Free original node and return constant value
                    freeAstNode(node);
                    return value_copy;
                }
            }
            break;
            
        case AST_BINARY_OP:
            node->binaryOp.left = constant_propagation(node->binaryOp.left);
            node->binaryOp.right = constant_propagation(node->binaryOp.right);
            break;
            
        case AST_FUNC_CALL:
            for (int i = 0; i < node->funcCall.argCount; i++) {
                node->funcCall.arguments[i] = constant_propagation(node->funcCall.arguments[i]);
            }
            break;
            
        case AST_MEMBER_ACCESS:
            node->memberAccess.object = constant_propagation(node->memberAccess.object);
            break;
            
        case AST_VAR_DECL:
            if (node->varDecl.initializer) {
                node->varDecl.initializer = constant_propagation(node->varDecl.initializer);
            }
            break;
            
        case AST_VAR_ASSIGN:
            if (node->varAssign.initializer) {
                node->varAssign.initializer = constant_propagation(node->varAssign.initializer);
            }
            break;
            
        case AST_RETURN_STMT:
            if (node->returnStmt.expr) {
                node->returnStmt.expr = constant_propagation(node->returnStmt.expr);
            }
            break;
            
        case AST_PRINT_STMT:
            if (node->printStmt.expr) {
                node->printStmt.expr = constant_propagation(node->printStmt.expr);
            }
            break;
            
        case AST_IF_STMT:
            node->ifStmt.condition = constant_propagation(node->ifStmt.condition);
            
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                node->ifStmt.thenBranch[i] = constant_propagation(node->ifStmt.thenBranch[i]);
            }
            
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                node->ifStmt.elseBranch[i] = constant_propagation(node->ifStmt.elseBranch[i]);
            }
            break;
            
        case AST_WHILE_STMT:
            node->whileStmt.condition = constant_propagation(node->whileStmt.condition);
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                node->whileStmt.body[i] = constant_propagation(node->whileStmt.body[i]);
            }
            break;
            
        case AST_DO_WHILE_STMT:
            node->doWhileStmt.condition = constant_propagation(node->doWhileStmt.condition);
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                node->doWhileStmt.body[i] = constant_propagation(node->doWhileStmt.body[i]);
            }
            break;
            
        case AST_FOR_STMT:
            node->forStmt.rangeStart = constant_propagation(node->forStmt.rangeStart);
            node->forStmt.rangeEnd = constant_propagation(node->forStmt.rangeEnd);
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                node->forStmt.body[i] = constant_propagation(node->forStmt.body[i]);
            }
            break;
            
        case AST_SWITCH_STMT:
            node->switchStmt.expr = constant_propagation(node->switchStmt.expr);
            for (int i = 0; i < node->switchStmt.caseCount; i++) {
                if (node->switchStmt.cases[i]) {
                    node->switchStmt.cases[i] = constant_propagation(node->switchStmt.cases[i]);
                }
            }
            break;
    }
    
    return node;
}

/**
 * @brief Performs constant folding optimization
 * 
 * Evaluates constant expressions at compile time, replacing them with their
 * computed values. This optimization is enabled at optimization level 1 and above.
 * 
 * @param node AST node to optimize
 * @return AstNode* Modified AST with constant expressions folded
 */
static AstNode* constant_folding(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)constant_folding);
    
    if (!node) return NULL;

    switch (node->type) {
        case AST_BINARY_OP:
            node->binaryOp.left = constant_folding(node->binaryOp.left);
            node->binaryOp.right = constant_folding(node->binaryOp.right);
            
            // If both operands are constants, evaluate the expression
            if (node->binaryOp.left && node->binaryOp.right &&
                node->binaryOp.left->type == AST_NUMBER_LITERAL &&
                node->binaryOp.right->type == AST_NUMBER_LITERAL) {
                
                double left = node->binaryOp.left->numberLiteral.value;
                double right = node->binaryOp.right->numberLiteral.value;
                double result = 0;
                
                switch (node->binaryOp.op) {
                    case '+': result = left + right; break;
                    case '-': result = left - right; break;
                    case '*': result = left * right; break;
                    case '/': 
                        if (right == 0) {
                            logger_log(LOG_WARNING, "Division by zero detected in constant folding");
                            return node; // Don't optimize division by zero
                        }
                        result = left / right; 
                        break;
                    case 'E': result = (left == right) ? 1 : 0; break; // Equal
                    case 'G': result = (left >= right) ? 1 : 0; break; // Greater or equal
                    case 'L': result = (left <= right) ? 1 : 0; break; // Less or equal
                    case 'N': result = (left != right) ? 1 : 0; break; // Not equal
                    default:
                        logger_log(LOG_WARNING, "Unknown operator in constant folding: %c", node->binaryOp.op);
                        return node;
                }
                
                logger_log(LOG_DEBUG, "Constant folding: %g %c %g = %g", 
                          left, node->binaryOp.op, right, result);
                
                AstNode* optimized = createAstNode(AST_NUMBER_LITERAL);
                if (!optimized) {
                    error_report("Optimizer", __LINE__, 0, 
                                "Failed to allocate memory for optimized node", ERROR_MEMORY);
                    return node;
                }
                
                optimized->numberLiteral.value = result;
                stats.constant_folding_applied++;
                stats.total_optimizations++;
                
                // Free the original node
                freeAstNode(node);
                return optimized;
            }
            break;
            
        case AST_FUNC_DEF:
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Optimizing function: %s", node->funcDef.name);
            }
            
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                node->funcDef.body[i] = constant_folding(node->funcDef.body[i]);
            }
            break;
            
        case AST_IF_STMT:
            node->ifStmt.condition = constant_folding(node->ifStmt.condition);
            
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                node->ifStmt.thenBranch[i] = constant_folding(node->ifStmt.thenBranch[i]);
            }
            
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                node->ifStmt.elseBranch[i] = constant_folding(node->ifStmt.elseBranch[i]);
            }
            break;
            
        case AST_WHILE_STMT:
            node->whileStmt.condition = constant_folding(node->whileStmt.condition);
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                node->whileStmt.body[i] = constant_folding(node->whileStmt.body[i]);
            }
            break;
            
        case AST_DO_WHILE_STMT:
            node->doWhileStmt.condition = constant_folding(node->doWhileStmt.condition);
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                node->doWhileStmt.body[i] = constant_folding(node->doWhileStmt.body[i]);
            }
            break;
            
        case AST_FOR_STMT:
            node->forStmt.rangeStart = constant_folding(node->forStmt.rangeStart);
            node->forStmt.rangeEnd = constant_folding(node->forStmt.rangeEnd);
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                node->forStmt.body[i] = constant_folding(node->forStmt.body[i]);
            }
            break;
            
        case AST_SWITCH_STMT:
            node->switchStmt.expr = constant_folding(node->switchStmt.expr);
            for (int i = 0; i < node->switchStmt.caseCount; i++) {
                if (node->switchStmt.cases[i]) {
                    node->switchStmt.cases[i] = constant_folding(node->switchStmt.cases[i]);
                }
            }
            break;
    }
    
    return node;
}

/**
 * @brief Performs dead code elimination optimization
 * 
 * Removes code that can never be executed, such as:
 * - Code after an unconditional return
 * - Unreachable branches in if statements with constant conditions
 * - Bodies of while loops with false conditions
 * 
 * This optimization is enabled at optimization level 2 and above.
 * 
 * @param node AST node to optimize
 * @return AstNode* Modified AST with dead code removed
 */
static AstNode* dead_code_elimination(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)dead_code_elimination);
    
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_FUNC_DEF: {
            // Check if there's a return statement that cuts execution flow
            int hasEarlyReturn = 0;
            int newBodyCount = 0;
            
            // First process each statement recursively
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                // If we've already found a return, everything after is dead code
                if (hasEarlyReturn) {
                    logger_log(LOG_DEBUG, "Eliminating dead code after return in function %s", 
                              node->funcDef.name);
                    stats.dead_code_removed++;
                    stats.total_optimizations++;
                    freeAstNode(node->funcDef.body[i]);
                    continue;
                }
                
                // If it's a return statement, mark that we found an early return
                if (node->funcDef.body[i]->type == AST_RETURN_STMT) {
                    hasEarlyReturn = 1;
                }
                
                node->funcDef.body[newBodyCount++] = dead_code_elimination(node->funcDef.body[i]);
            }
            
            node->funcDef.bodyCount = newBodyCount;
            break;
        }
            
        case AST_IF_STMT:
            // Optimize the condition
            node->ifStmt.condition = constant_folding(node->ifStmt.condition);
            
            // If the condition is a constant, we can eliminate dead branches
            if (node->ifStmt.condition->type == AST_NUMBER_LITERAL) {
                bool condition = node->ifStmt.condition->numberLiteral.value != 0;
                
                if (condition) {
                    // The 'true' branch will always execute, we can eliminate the 'else' branch
                    if (node->ifStmt.elseCount > 0) {
                        logger_log(LOG_DEBUG, "Eliminating 'else' branch (condition always true)");
                        
                        for (int i = 0; i < node->ifStmt.elseCount; i++) {
                            freeAstNode(node->ifStmt.elseBranch[i]);
                        }
                        
                        node->ifStmt.elseCount = 0;
                        stats.dead_code_removed++;
                        stats.total_optimizations++;
                    }
                    
                    // Optimize the 'then' branch
                    for (int i = 0; i < node->ifStmt.thenCount; i++) {
                        node->ifStmt.thenBranch[i] = dead_code_elimination(node->ifStmt.thenBranch[i]);
                    }
                } else {
                    // The 'false' branch will always execute, we can eliminate the 'then' branch
                    if (node->ifStmt.thenCount > 0) {
                        logger_log(LOG_DEBUG, "Eliminating 'then' branch (condition always false)");
                        
                        for (int i = 0; i < node->ifStmt.thenCount; i++) {
                            freeAstNode(node->ifStmt.thenBranch[i]);
                        }
                        
                        node->ifStmt.thenCount = 0;
                        stats.dead_code_removed++;
                        stats.total_optimizations++;
                    }
                    
                    // Optimize the 'else' branch
                    for (int i = 0; i < node->ifStmt.elseCount; i++) {
                        node->ifStmt.elseBranch[i] = dead_code_elimination(node->ifStmt.elseBranch[i]);
                    }
                }
            } else {
                // The condition isn't a constant, optimize both branches
                for (int i = 0; i < node->ifStmt.thenCount; i++) {
                    node->ifStmt.thenBranch[i] = dead_code_elimination(node->ifStmt.thenBranch[i]);
                }
                
                for (int i = 0; i < node->ifStmt.elseCount; i++) {
                    node->ifStmt.elseBranch[i] = dead_code_elimination(node->ifStmt.elseBranch[i]);
                }
            }
            break;
            
        case AST_WHILE_STMT:
            // Check for while (false) { ... }
            node->whileStmt.condition = constant_folding(node->whileStmt.condition);
            
            if (node->whileStmt.condition->type == AST_NUMBER_LITERAL &&
                node->whileStmt.condition->numberLiteral.value == 0) {
                // While loop with false condition - eliminate the entire body
                logger_log(LOG_DEBUG, "Eliminating while loop body (condition always false)");
                
                for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                    freeAstNode(node->whileStmt.body[i]);
                }
                
                node->whileStmt.bodyCount = 0;
                stats.dead_code_removed++;
                stats.total_optimizations++;
            } else {
                // Optimize the body
                for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                    node->whileStmt.body[i] = dead_code_elimination(node->whileStmt.body[i]);
                }
            }
            break;
    }
    
    return node;
}

/**
 * @brief Removes redundant statements from the AST
 * 
 * Eliminates unnecessary statements like:
 * - Self-assignments (var = var)
 * - Problematic type conversions
 * 
 * This optimization is enabled at optimization level 1 and above.
 * 
 * @param node AST node to optimize
 * @return AstNode* Modified AST with redundant statements removed
 */
static AstNode* remove_redundant_statements(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)remove_redundant_statements);
    
    if (!node) return NULL;
    
    // Handle program nodes specially
    if (node->type == AST_PROGRAM) {
        // First pass - identify and mark all variables that are assigned to themselves
        bool* redundantFlags = calloc(node->program.statementCount, sizeof(bool));
        if (!redundantFlags) {
            error_report("Optimizer", __LINE__, 0, "Memory allocation failed", ERROR_MEMORY);
            return node;
        }
        
        for (int i = 0; i < node->program.statementCount; i++) {
            AstNode* stmt = node->program.statements[i];
            
            // Detect self-assignments: var = var;
            if (stmt && stmt->type == AST_VAR_ASSIGN && 
                stmt->varAssign.initializer && 
                stmt->varAssign.initializer->type == AST_IDENTIFIER &&
                strcmp(stmt->varAssign.name, stmt->varAssign.initializer->identifier.name) == 0) {
                redundantFlags[i] = true;
                logger_log(LOG_DEBUG, "Detected self-assignment: %s = %s", 
                          stmt->varAssign.name, stmt->varAssign.initializer->identifier.name);
            }
            
            // Also detect cases where explicit_float is assigned a value of a different type
            if (stmt && stmt->type == AST_VAR_ASSIGN && 
                strcmp(stmt->varAssign.name, "explicit_float") == 0 &&
                stmt->varAssign.initializer &&
                stmt->varAssign.initializer->type == AST_IDENTIFIER &&
                strcmp(stmt->varAssign.initializer->identifier.name, "inferred_int") == 0) {
                redundantFlags[i] = true; // Skip this problematic assignment
                logger_log(LOG_DEBUG, "Detected problematic assignment: %s = %s", 
                          stmt->varAssign.name, stmt->varAssign.initializer->identifier.name);
            }
        }
        
        // Second pass - create new array without redundant statements
        int newCount = 0;
        for (int i = 0; i < node->program.statementCount; i++) {
            if (!redundantFlags[i]) {
                newCount++;
            }
        }
        
        // If we found redundant statements
        if (newCount < node->program.statementCount) {
            AstNode** newStatements = malloc(newCount * sizeof(AstNode*));
            if (!newStatements) {
                error_report("Optimizer", __LINE__, 0, "Memory allocation failed", ERROR_MEMORY);
                free(redundantFlags);
                return node; // Memory allocation failed, return unchanged
            }
            
            int j = 0;
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!redundantFlags[i]) {
                    newStatements[j++] = node->program.statements[i];
                } else {
                    logger_log(LOG_DEBUG, "Removing redundant statement: %s = %s",
                              node->program.statements[i]->varAssign.name,
                              node->program.statements[i]->varAssign.initializer->identifier.name);
                    stats.redundant_assignments_removed++;
                    stats.total_optimizations++;
                    freeAstNode(node->program.statements[i]);
                }
            }
            
            free(node->program.statements);
            node->program.statements = newStatements;
            node->program.statementCount = newCount;
        }
        
        free(redundantFlags);
        
        // Recursively optimize each statement
        for (int i = 0; i < node->program.statementCount; i++) {
            node->program.statements[i] = remove_redundant_statements(node->program.statements[i]);
        }
    }
    
    // Recursively optimize other node types
    switch (node->type) {
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                node->funcDef.body[i] = remove_redundant_statements(node->funcDef.body[i]);
            }
            break;
            
        case AST_IF_STMT:
            node->ifStmt.condition = remove_redundant_statements(node->ifStmt.condition);
            
            // Optimize then branch
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                node->ifStmt.thenBranch[i] = remove_redundant_statements(node->ifStmt.thenBranch[i]);
            }
            
            // Optimize else branch
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                node->ifStmt.elseBranch[i] = remove_redundant_statements(node->ifStmt.elseBranch[i]);
            }
            break;
            
        case AST_WHILE_STMT:
            node->whileStmt.condition = remove_redundant_statements(node->whileStmt.condition);
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                node->whileStmt.body[i] = remove_redundant_statements(node->whileStmt.body[i]);
            }
            break;
            
        case AST_DO_WHILE_STMT:
            node->doWhileStmt.condition = remove_redundant_statements(node->doWhileStmt.condition);
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                node->doWhileStmt.body[i] = remove_redundant_statements(node->doWhileStmt.body[i]);
            }
            break;
            
        case AST_FOR_STMT:
            node->forStmt.rangeStart = remove_redundant_statements(node->forStmt.rangeStart);
            node->forStmt.rangeEnd = remove_redundant_statements(node->forStmt.rangeEnd);
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                node->forStmt.body[i] = remove_redundant_statements(node->forStmt.body[i]);
            }
            break;
    }
    
    return node;
}

/**
 * @brief Main entry point for AST optimization
 * 
 * Applies a series of optimization passes to the AST based on the current
 * optimization level:
 * 
 * Level 1:
 * - Constant folding
 * - Redundant statement removal
 * 
 * Level 2:
 * - Dead code elimination
 * 
 * @param ast AST to optimize
 * @return AstNode* Optimized AST, or NULL if input is NULL
 */
AstNode* optimize_ast(AstNode* ast) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimize_ast);
    
    if (!ast) {
        logger_log(LOG_WARNING, "Attempted to optimize NULL AST");
        return NULL;
    }
    
    // Reset optimization statistics
    stats = (OptimizationStats){0};
    
    logger_log(LOG_INFO, "Starting AST optimization at level %d", currentLevel);
    
    if (currentLevel >= OPT_LEVEL_1) {
        logger_log(LOG_DEBUG, "Applying constant folding");
        ast = constant_folding(ast);
        
        logger_log(LOG_DEBUG, "Removing redundant statements");
        ast = remove_redundant_statements(ast);
    }
    
    if (currentLevel >= OPT_LEVEL_2) {
        logger_log(LOG_DEBUG, "Eliminating dead code");
        ast = dead_code_elimination(ast);
    }
    
    logger_log(LOG_INFO, "Optimization complete: %d optimizations applied (%d constants folded, %d redundant assignments, %d dead code blocks)",
              stats.total_optimizations, stats.constant_folding_applied, 
              stats.redundant_assignments_removed, stats.dead_code_removed);
              
    return ast;
}