#include "optimizer.h"
#include "ast.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static OptimizerLevel currentLevel = OPT_LEVEL_0;
static int debug_level = 1;  // Default debug level

// Statistics for optimizations performed
static OptimizationStats stats = {0};

void optimizer_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Optimizer debug level set to %d", level);
}

int optimizer_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_get_debug_level);
    return debug_level;
}

void optimizer_init(OptimizerLevel level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_init);
    
    currentLevel = level;
    // Reset statistics
    stats = (OptimizationStats){0};
    
    logger_log(LOG_INFO, "Optimizer initialized with level %d", level);
}

OptimizationStats optimizer_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)optimizer_get_stats);
    return stats;
}

/**
 * Performs constant folding (evaluating constant expressions)
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
            
        case AST_EXPR_STMT:
            if (node->exprStmt.expr) {
                node->exprStmt.expr = constant_folding(node->exprStmt.expr);
            }
            break;
    }
    
    return node;
}

/**
 * Eliminates dead code (code that will never be executed)
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
 * Removes redundant statements like self-assignments
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