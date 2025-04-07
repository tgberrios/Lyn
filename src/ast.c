#include "ast.h"
#include "memory.h"   // Uses malloc/free or custom memory functions
#include "error.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>    // For fprintf, stderr
#include <stdint.h>   // For uintptr_t
#include <stdbool.h>

/**
 * @file ast.c
 * @brief Implementation of the Abstract Syntax Tree (AST) system
 * 
 * This file implements the core functionality for managing Abstract Syntax Trees
 * in the Lyn compiler. It provides functions for creating, manipulating, and
 * managing AST nodes, including memory management, debugging, and tree traversal.
 */

// Debug level: 0=minimum, 3=maximum
static int debug_level = 1;

// AST usage statistics
static AstStats stats = {0};

/**
 * @brief Initializes the AST system
 * 
 * This function initializes the AST system by resetting all statistics
 * and setting up the initial state. It should be called before any AST
 * operations are performed.
 */
void ast_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_init);
    
    // Reset statistics
    stats.nodes_created = 0;
    stats.nodes_freed = 0;
    stats.max_depth = 0;
    stats.memory_used = 0;
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "AST system initialized");
    }
}

/**
 * @brief Cleans up and frees resources from the AST system
 * 
 * This function performs cleanup operations on the AST system,
 * logging final statistics about node creation and memory usage.
 */
void ast_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_cleanup);
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "AST system cleanup completed. Stats: created=%d, freed=%d, max_depth=%d", 
                  stats.nodes_created, stats.nodes_freed, stats.max_depth);
    }
}

/**
 * @brief Sets the debug level for the AST system
 * 
 * @param level The new debug level (0=minimum, 3=maximum)
 */
void ast_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_set_debug_level);
    
    debug_level = level;
    logger_log(LOG_INFO, "AST debug level set to %d", level);
}

/**
 * @brief Gets the current debug level
 * 
 * @return int The current debug level
 */
int ast_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_get_debug_level);
    
    return debug_level;
}

/**
 * @brief Gets AST node usage statistics
 * 
 * @return AstStats Current statistics about AST node usage
 */
AstStats ast_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_get_stats);
    
    return stats;
}

/**
 * @brief Creates a new AST node of the specified type
 * 
 * This function allocates and initializes a new AST node with the given type.
 * It also updates the AST statistics and performs memory allocation checks.
 * 
 * @param type The type of AST node to create
 * @return AstNode* The newly created node, or NULL if allocation fails
 */
AstNode* createAstNode(AstNodeType type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)createAstNode);
    
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    if (!node) {
        error_report("AST", __LINE__, 0, "Failed to allocate memory for AST node", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for AST node of type %d", type);
        return NULL;
    }
    
    node->type = type;
    node->inferredType = NULL;  // No inferred type initially
    
    stats.nodes_created++;
    stats.memory_used += sizeof(AstNode);
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Created AST node of type %d (%s)", type, astNodeTypeToString(type));
    }
    
    return node;
}

/**
 * @brief Determines if a node has dynamically allocated memory
 * 
 * This helper function checks if an AST node has any dynamically allocated
 * memory that needs to be freed.
 * 
 * @param node The AST node to check
 * @return bool true if the node has allocated memory, false otherwise
 */
static bool hasAllocatedMemory(AstNode* node) {
    if (!node) return false;
    
    switch (node->type) {
        case AST_PROGRAM:
            return node->program.statements != NULL;
        case AST_FUNC_DEF:
            return node->funcDef.parameters != NULL || node->funcDef.body != NULL;
        case AST_CLASS_DEF:
            return node->classDef.members != NULL;
        case AST_MODULE_DECL:
            return node->moduleDecl.declarations != NULL;
        case AST_BLOCK:
            return node->block.statements != NULL;
        case AST_IF_STMT:
            return node->ifStmt.thenBranch != NULL || node->ifStmt.elseBranch != NULL;
        case AST_FOR_STMT:
            return node->forStmt.body != NULL;
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            return node->whileStmt.body != NULL;
        case AST_SWITCH_STMT:
            return node->switchStmt.cases != NULL || node->switchStmt.defaultCase != NULL;
        case AST_CASE_STMT:
            return node->caseStmt.body != NULL;
        case AST_TRY_CATCH_STMT:
            return node->tryCatchStmt.tryBody != NULL || 
                   node->tryCatchStmt.catchBody != NULL || 
                   node->tryCatchStmt.finallyBody != NULL;
        case AST_ARRAY_LITERAL:
            return node->arrayLiteral.elements != NULL;
        case AST_FUNC_CALL:
            return node->funcCall.arguments != NULL;
        case AST_LAMBDA:
            return node->lambda.parameters != NULL;
        case AST_CURRY_EXPR:
            return node->curryExpr.appliedArgs != NULL;
        case AST_ASPECT_DEF:
            return true; // Memory is assumed to be allocated in pointcuts/advice arrays
        case AST_ADVICE:
            return node->advice.body != NULL;
        case AST_PATTERN_MATCH:
            return node->patternMatch.cases != NULL;
        case AST_PATTERN_CASE:
            return node->patternCase.body != NULL;
        default:
            return false;
    }
}

/**
 * @brief Frees an AST node and all its children
 * 
 * This function recursively frees an AST node and all its child nodes,
 * ensuring proper memory cleanup. It includes safety checks for invalid
 * memory addresses and node types.
 * 
 * @param node The AST node to free
 */
void freeAstNode(AstNode* node) {
    if (!node) return;
    
    uintptr_t node_addr = (uintptr_t)node;
    if (node_addr < 0x1000 || 
        node_addr > (uintptr_t)0x7fffffffffffffff ||
        (node_addr & 0x7) != 0 ||
        (node_addr >= 0x2000000000 && node_addr <= 0x20ffffffffff) ||
        node_addr == 0x200a202020202020 ||
        node_addr == 0x200a3b2928746e69) {
        logger_log(LOG_WARNING, "Skipping invalid AST node address %p", (void*)node);
        return;
    }
    
    AstNodeType type;
    if (!memcpy(&type, &node->type, sizeof(AstNodeType)) || 
        (type < AST_PROGRAM || type > AST_PATTERN_CASE)) {
        logger_log(LOG_WARNING, "Detected invalid node type %d at address %p", type, (void*)node);
        return;
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)freeAstNode);
    
    // Free child nodes based on node type
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                freeAstNode(node->program.statements[i]);
            }
            free(node->program.statements);
            break;
        case AST_NUMBER_LITERAL:
            break;
        case AST_STRING_LITERAL:
            break;
        case AST_IDENTIFIER:
            break;
        case AST_VAR_DECL:
            freeAstNode(node->varDecl.initializer);
            break;
        case AST_VAR_ASSIGN:
            freeAstNode(node->varAssign.initializer);
            break;
        case AST_FUNC_DEF:
            if (node->funcDef.parameters) {
                for (int i = 0; i < node->funcDef.paramCount; i++) {
                    freeAstNode(node->funcDef.parameters[i]);
                }
                free(node->funcDef.parameters);
            }
            if (node->funcDef.body) {
                for (int i = 0; i < node->funcDef.bodyCount; i++) {
                    freeAstNode(node->funcDef.body[i]);
                }
                free(node->funcDef.body);
            }
            break;
        case AST_IF_STMT:
            freeAstNode(node->ifStmt.condition);
            if (node->ifStmt.thenBranch) {
                for (int i = 0; i < node->ifStmt.thenCount; i++) {
                    freeAstNode(node->ifStmt.thenBranch[i]);
                }
                free(node->ifStmt.thenBranch);
            }
            if (node->ifStmt.elseBranch) {
                for (int i = 0; i < node->ifStmt.elseCount; i++) {
                    freeAstNode(node->ifStmt.elseBranch[i]);
                }
                free(node->ifStmt.elseBranch);
            }
            break;
        case AST_WHILE_STMT:
            freeAstNode(node->whileStmt.condition);
            if (node->whileStmt.body) {
                for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                    freeAstNode(node->whileStmt.body[i]);
                }
                free(node->whileStmt.body);
            }
            break;
        case AST_FOR_STMT:
            freeAstNode(node->forStmt.rangeStart);
            freeAstNode(node->forStmt.rangeEnd);
            if (node->forStmt.body) {
                for (int i = 0; i < node->forStmt.bodyCount; i++) {
                    freeAstNode(node->forStmt.body[i]);
                }
                free(node->forStmt.body);
            }
            break;
        case AST_RETURN_STMT:
            freeAstNode(node->returnStmt.expr);
            break;
        case AST_BINARY_OP:
            freeAstNode(node->binaryOp.left);
            freeAstNode(node->binaryOp.right);
            break;
        case AST_UNARY_OP:
            freeAstNode(node->unaryOp.expr);
            break;
        case AST_FUNC_CALL:
            if (node->funcCall.arguments) {
                for (int i = 0; i < node->funcCall.argCount; i++) {
                    freeAstNode(node->funcCall.arguments[i]);
                }
                free(node->funcCall.arguments);
            }
            break;
        case AST_MEMBER_ACCESS:
            if (node->memberAccess.object) {
                freeAstNode(node->memberAccess.object);
                node->memberAccess.object = NULL;
            }
            break;
        case AST_PRINT_STMT:
            freeAstNode(node->printStmt.expr);
            break;
        case AST_CLASS_DEF:
            if (node->classDef.members) {
                for (int i = 0; i < node->classDef.memberCount; i++) {
                    freeAstNode(node->classDef.members[i]);
                }
                free(node->classDef.members);
            }
            break;
        case AST_LAMBDA:
            if (node->lambda.parameters) {
                for (int i = 0; i < node->lambda.paramCount; i++) {
                    freeAstNode(node->lambda.parameters[i]);
                }
                free(node->lambda.parameters);
            }
            freeAstNode(node->lambda.body);
            break;
        case AST_ARRAY_LITERAL:
            if (node->arrayLiteral.elements) {
                for (int i = 0; i < node->arrayLiteral.elementCount; i++) {
                    freeAstNode(node->arrayLiteral.elements[i]);
                }
                free(node->arrayLiteral.elements);
            }
            break;
        case AST_MODULE_DECL:
            if (node->moduleDecl.declarations) {
                for (int i = 0; i < node->moduleDecl.declarationCount; i++) {
                    freeAstNode(node->moduleDecl.declarations[i]);
                }
                free(node->moduleDecl.declarations);
            }
            break;
        case AST_IMPORT:
            // Liberar memoria para importaciones selectivas
            if (node->importStmt.hasSymbolList) {
                for (int i = 0; i < node->importStmt.symbolCount; i++) {
                    if (node->importStmt.symbols && node->importStmt.symbols[i])
                        memory_free((void*)node->importStmt.symbols[i]);
                    if (node->importStmt.aliases && node->importStmt.aliases[i])
                        memory_free((void*)node->importStmt.aliases[i]);
                }
                memory_free((void*)node->importStmt.symbols);
                memory_free((void*)node->importStmt.aliases);
            }
            break;
        case AST_DO_WHILE_STMT:
            freeAstNode(node->doWhileStmt.condition);
            if (node->doWhileStmt.body) {
                for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                    freeAstNode(node->doWhileStmt.body[i]);
                }
                free(node->doWhileStmt.body);
            }
            break;
        case AST_SWITCH_STMT:
            freeAstNode(node->switchStmt.expr);
            if (node->switchStmt.cases) {
                for (int i = 0; i < node->switchStmt.caseCount; i++) {
                    freeAstNode(node->switchStmt.cases[i]);
                }
                free(node->switchStmt.cases);
            }
            if (node->switchStmt.defaultCase) {
                for (int i = 0; i < node->switchStmt.defaultCaseCount; i++) {
                    freeAstNode(node->switchStmt.defaultCase[i]);
                }
                free(node->switchStmt.defaultCase);
            }
            break;
        case AST_CASE_STMT:
            freeAstNode(node->caseStmt.expr);
            if (node->caseStmt.body) {
                for (int i = 0; i < node->caseStmt.bodyCount; i++) {
                    freeAstNode(node->caseStmt.body[i]);
                }
                free(node->caseStmt.body);
            }
            break;
        case AST_TRY_CATCH_STMT:
            if (node->tryCatchStmt.tryBody) {
                for (int i = 0; i < node->tryCatchStmt.tryCount; i++) {
                    freeAstNode(node->tryCatchStmt.tryBody[i]);
                }
                free(node->tryCatchStmt.tryBody);
            }
            if (node->tryCatchStmt.catchBody) {
                for (int i = 0; i < node->tryCatchStmt.catchCount; i++) {
                    freeAstNode(node->tryCatchStmt.catchBody[i]);
                }
                free(node->tryCatchStmt.catchBody);
            }
            if (node->tryCatchStmt.finallyBody) {
                for (int i = 0; i < node->tryCatchStmt.finallyCount; i++) {
                    freeAstNode(node->tryCatchStmt.finallyBody[i]);
                }
                free(node->tryCatchStmt.finallyBody);
            }
            break;
        case AST_THROW_STMT:
            freeAstNode(node->throwStmt.expr);
            break;
        case AST_BREAK_STMT:
            break;
        case AST_CONTINUE_STMT:
            break;
        case AST_CURRY_EXPR:
            freeAstNode(node->curryExpr.baseFunc);
            if (node->curryExpr.appliedArgs) {
                for (int i = 0; i < node->curryExpr.appliedCount; i++) {
                    freeAstNode(node->curryExpr.appliedArgs[i]);
                }
                free(node->curryExpr.appliedArgs);
            }
            break;
        case AST_NEW_EXPR:
            if (node->newExpr.arguments) {
                for (int i = 0; i < node->newExpr.argCount; i++) {
                    freeAstNode(node->newExpr.arguments[i]);
                }
                free(node->newExpr.arguments);
            }
            break;
        case AST_THIS_EXPR:
            // No hay campos dinámicos para liberar en 'this'
            break;
        case AST_FUNC_COMPOSE:
            freeAstNode(node->funcCompose.left);
            freeAstNode(node->funcCompose.right);
            break;
        case AST_POINTCUT:
            break;
        case AST_ADVICE:
            if (node->advice.body) {
                for (int i = 0; i < node->advice.bodyCount; i++) {
                    freeAstNode(node->advice.body[i]);
                }
                free(node->advice.body);
            }
            break;
        case AST_ASPECT_DEF:
            if (node->aspectDef.pointcuts) {
                for (int i = 0; i < node->aspectDef.pointcutCount; i++) {
                    freeAstNode(node->aspectDef.pointcuts[i]);
                }
                free(node->aspectDef.pointcuts);
            }
            if (node->aspectDef.advice) {
                for (int i = 0; i < node->aspectDef.adviceCount; i++) {
                    freeAstNode(node->aspectDef.advice[i]);
                }
                free(node->aspectDef.advice);
            }
            break;
        case AST_PATTERN_MATCH:
            freeAstNode(node->patternMatch.expr);
            if (node->patternMatch.cases) {
                for (int i = 0; i < node->patternMatch.caseCount; i++) {
                    freeAstNode(node->patternMatch.cases[i]);
                }
                free(node->patternMatch.cases);
            }
            if (node->patternMatch.otherwise) {
                freeAstNode(node->patternMatch.otherwise);
            }
            break;
        case AST_PATTERN_CASE:
            freeAstNode(node->patternCase.pattern);
            if (node->patternCase.body) {
                for (int i = 0; i < node->patternCase.bodyCount; i++) {
                    freeAstNode(node->patternCase.body[i]);
                }
                free(node->patternCase.body);
            }
            break;
        default:
            break;
    }
    free(node);
    stats.nodes_freed++;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Freed AST node of type %d", node->type);
    }
}

/**
 * @brief Frees an entire AST tree
 * 
 * This function frees the entire AST tree starting from the root node.
 * 
 * @param root The root node of the AST tree to free
 */
void freeAst(AstNode* root) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)freeAst);
    logger_log(LOG_DEBUG, "Starting to free AST tree");
    freeAstNode(root);
}

/**
 * @brief Frees a complete AST program
 * 
 * This function frees an AST program node and all its contents,
 * ensuring proper cleanup of all program-related nodes.
 * 
 * @param program The program node to free
 */
void freeAstProgram(AstNode* program) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)freeAstProgram);
    
    if (!program || program->type != AST_PROGRAM) {
        logger_log(LOG_WARNING, "Attempted to free non-program node as program");
        return;
    }
    
    freeAstNode(program);
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "Freed AST program. Stats: created=%d, freed=%d", 
                 stats.nodes_created, stats.nodes_freed);
    }
}

/**
 * @brief Prints an AST node recursively with indentation for debugging
 * 
 * This function provides a human-readable representation of the AST,
 * useful for debugging and understanding the program structure.
 * 
 * @param node The AST node to print
 * @param indent The current indentation level
 */
void printAst(AstNode* node, int indent) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)printAst);
    
    if (!node) return;
    
    if (indent > stats.max_depth) {
        stats.max_depth = indent;
    }
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    const char* typeStr = astNodeTypeToString(node->type);
    
    // Print node based on its type
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (%d statements)\n", node->program.statementCount);
            for (int i = 0; i < node->program.statementCount; i++) {
                printAst(node->program.statements[i], indent + 1);
            }
            break;
        case AST_FUNC_DEF:
            printf("FuncDef: '%s' (%d params, %d statements)\n", 
                  node->funcDef.name, node->funcDef.paramCount, node->funcDef.bodyCount);
            for (int i = 0; i < node->funcDef.paramCount; i++) {
                printAst(node->funcDef.parameters[i], indent + 1);
            }
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                printAst(node->funcDef.body[i], indent + 1);
            }
            break;
        case AST_CLASS_DEF:
            if (strlen(node->classDef.baseClassName) > 0) {
                printf("ClassDef: '%s' extends '%s' (%d members)\n", 
                      node->classDef.name, node->classDef.baseClassName, node->classDef.memberCount);
            } else {
                printf("ClassDef: '%s' (%d members)\n", 
                      node->classDef.name, node->classDef.memberCount);
            }
            for (int i = 0; i < node->classDef.memberCount; i++) {
                printAst(node->classDef.members[i], indent + 1);
            }
            break;
        case AST_VAR_DECL:
            printf("VarDecl: '%s' type:'%s'\n", node->varDecl.name, node->varDecl.type);
            if (node->varDecl.initializer) {
                printAst(node->varDecl.initializer, indent + 1);
            }
            break;
        case AST_VAR_ASSIGN:
            printf("VarAssign: '%s'\n", node->varAssign.name);
            if (node->varAssign.initializer) {
                printAst(node->varAssign.initializer, indent + 1);
            }
            break;
        case AST_PRINT_STMT:
            printf("PrintStmt:\n");
            if (node->printStmt.expr) {
                printAst(node->printStmt.expr, indent + 1);
            }
            break;
        case AST_RETURN_STMT:
            printf("ReturnStmt:\n");
            if (node->returnStmt.expr) {
                printAst(node->returnStmt.expr, indent + 1);
            }
            break;
        case AST_BREAK_STMT:
            printf("BreakStmt\n");
            break;
        case AST_CONTINUE_STMT:
            printf("ContinueStmt\n");
            break;
        case AST_NUMBER_LITERAL:
            printf("NumberLiteral: %g\n", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            printf("StringLiteral: \"%s\"\n", node->stringLiteral.value);
            break;
        case AST_BOOLEAN_LITERAL:
            printf("BooleanLiteral: %s\n", node->boolLiteral.value ? "true" : "false");
            break;
        case AST_NULL_LITERAL:
            printf("NullLiteral\n");
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->identifier.name);
            break;
        case AST_BINARY_OP:
            printf("BinaryOp: '%c'\n", node->binaryOp.op);
            printAst(node->binaryOp.left, indent + 1);
            printAst(node->binaryOp.right, indent + 1);
            break;
        case AST_UNARY_OP:
            printf("UnaryOp: '%c'\n", node->unaryOp.op);
            printAst(node->unaryOp.expr, indent + 1);
            break;
        case AST_FUNC_CALL:
            printf("FuncCall: '%s' (%d args)\n", node->funcCall.name, node->funcCall.argCount);
            for (int i = 0; i < node->funcCall.argCount; i++) {
                printAst(node->funcCall.arguments[i], indent + 1);
            }
            break;
        case AST_MEMBER_ACCESS:
            printf("MemberAccess: .%s\n", node->memberAccess.member);
            printAst(node->memberAccess.object, indent + 1);
            break;
        case AST_NEW_EXPR:
            printf("NewExpr: new %s (%d args)\n", node->newExpr.className, node->newExpr.argCount);
            for (int i = 0; i < node->newExpr.argCount; i++) {
                printAst(node->newExpr.arguments[i], indent + 1);
            }
            break;
        case AST_THIS_EXPR:
            printf("ThisExpr\n");
            break;
        case AST_FUNC_COMPOSE:
            printf("FuncCompose:\n");
            printAst(node->funcCompose.left, indent + 1);
            printAst(node->funcCompose.right, indent + 1);
            break;
        case AST_CURRY_EXPR:
            printf("CurryExpr: applied %d/%d\n", node->curryExpr.appliedCount, node->curryExpr.totalArgCount);
            printAst(node->curryExpr.baseFunc, indent + 1);
            for (int i = 0; i < node->curryExpr.appliedCount; i++) {
                printAst(node->curryExpr.appliedArgs[i], indent + 1);
            }
            break;
        case AST_POINTCUT:
            printf("Pointcut: '%s' pattern:'%s'\n", node->pointcut.name, node->pointcut.pattern);
            break;
        case AST_ADVICE:
            printf("Advice: type %d on pointcut '%s' (%d statements)\n", node->advice.type, node->advice.pointcutName, node->advice.bodyCount);
            for (int i = 0; i < node->advice.bodyCount; i++) {
                printAst(node->advice.body[i], indent + 1);
            }
            break;
        case AST_ASPECT_DEF:
            printf("AspectDef: '%s' (%d pointcuts, %d advices)\n", node->aspectDef.name, node->aspectDef.pointcutCount, node->aspectDef.adviceCount);
            for (int i = 0; i < node->aspectDef.pointcutCount; i++) {
                printAst(node->aspectDef.pointcuts[i], indent + 1);
            }
            for (int i = 0; i < node->aspectDef.adviceCount; i++) {
                printAst(node->aspectDef.advice[i], indent + 1);
            }
            break;
        case AST_PATTERN_MATCH:
            printf("PatternMatch:\n");
            printAst(node->patternMatch.expr, indent + 1);
            for (int i = 0; i < node->patternMatch.caseCount; i++) {
                printAst(node->patternMatch.cases[i], indent + 1);
            }
            if (node->patternMatch.otherwise) {
                printf("Otherwise:\n");
                printAst(node->patternMatch.otherwise, indent + 1);
            }
            break;
        case AST_PATTERN_CASE:
            printf("PatternCase:\n");
            printAst(node->patternCase.pattern, indent + 1);
            for (int i = 0; i < node->patternCase.bodyCount; i++) {
                printAst(node->patternCase.body[i], indent + 1);
            }
            break;
        default:
            printf("Node of type %d (%s)\n", node->type, typeStr);
            break;
    }
}

/**
 * @brief Creates a shallow copy of an AST node
 * 
 * This function creates a copy of an AST node without recursively
 * copying its children. It's useful for temporary node manipulation.
 * 
 * @param node The AST node to copy
 * @return AstNode* A copy of the node, or NULL if allocation fails
 */
AstNode* copyAstNode(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)copyAstNode);
    
    if (!node) return NULL;
    
    AstNode* copy = malloc(sizeof(AstNode));
    if (!copy) {
        error_report("AST", __LINE__, 0, "Failed to allocate memory for AST node copy", ERROR_MEMORY);
        return NULL;
    }
    
    memcpy(copy, node, sizeof(AstNode));
    return copy;
}

/**
 * @brief Converts an AST node type to its string representation
 * 
 * This function provides a human-readable string representation
 * of an AST node type for debugging purposes.
 * 
 * @param type The AST node type to convert
 * @return const char* String representation of the node type
 */
const char* astNodeTypeToString(AstNodeType type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)astNodeTypeToString);
    
    switch (type) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_FUNC_DEF: return "FUNC_DEF";
        case AST_CLASS_DEF: return "CLASS_DEF";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_IMPORT: return "IMPORT";
        case AST_MODULE_DECL: return "MODULE_DECL";
        case AST_ASPECT_DEF: return "ASPECT_DEF";
        case AST_BLOCK: return "BLOCK";
        case AST_IF_STMT: return "IF_STMT";
        case AST_FOR_STMT: return "FOR_STMT";
        case AST_WHILE_STMT: return "WHILE_STMT";
        case AST_DO_WHILE_STMT: return "DO_WHILE_STMT";
        case AST_SWITCH_STMT: return "SWITCH_STMT";
        case AST_CASE_STMT: return "CASE_STMT";
        case AST_RETURN_STMT: return "RETURN_STMT";
        case AST_VAR_ASSIGN: return "VAR_ASSIGN";
        case AST_PRINT_STMT: return "PRINT_STMT";
        case AST_BREAK_STMT: return "BREAK_STMT";
        case AST_CONTINUE_STMT: return "CONTINUE_STMT";
        case AST_TRY_CATCH_STMT: return "TRY_CATCH_STMT";
        case AST_THROW_STMT: return "THROW_STMT";
        case AST_BINARY_OP: return "BINARY_OP";
        case AST_UNARY_OP: return "UNARY_OP";
        case AST_NUMBER_LITERAL: return "NUMBER_LITERAL";
        case AST_STRING_LITERAL: return "STRING_LITERAL";
        case AST_BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case AST_NULL_LITERAL: return "NULL_LITERAL";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_MEMBER_ACCESS: return "MEMBER_ACCESS";
        case AST_ARRAY_ACCESS: return "ARRAY_ACCESS";
        case AST_ARRAY_LITERAL: return "ARRAY_LITERAL";
        case AST_FUNC_CALL: return "FUNC_CALL";
        case AST_LAMBDA: return "LAMBDA";
        case AST_FUNC_COMPOSE: return "FUNC_COMPOSE";
        case AST_CURRY_EXPR: return "CURRY_EXPR";
        case AST_NEW_EXPR: return "NEW_EXPR";
        case AST_THIS_EXPR: return "THIS_EXPR";
        case AST_POINTCUT: return "POINTCUT";
        case AST_ADVICE: return "ADVICE";
        case AST_PATTERN_MATCH: return "PATTERN_MATCH";
        case AST_PATTERN_CASE: return "PATTERN_CASE";
        default: return "UNKNOWN";
    }
}
