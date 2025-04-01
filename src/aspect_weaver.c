/**
 * @file aspect_weaver.c
 * @brief Implementation of the aspect weaving system for the Lyn language
 * 
 * This file implements the Aspect-Oriented Programming (AOP) system
 * for the Lyn language. The weaver is responsible for:
 * 1. Collecting all aspects defined in the program
 * 2. Identifying joinpoints that match the pointcuts
 * 3. Applying the corresponding advice at the joinpoints
 * 4. Maintaining weaving process statistics
 */

#include "aspect_weaver.h"
#include "logger.h"
#include "error.h"
#include "memory.h"
#include <string.h>
#include <stdlib.h>

// Usamos la enumeración AdviceType definida en ast.h en lugar de defines
#include "ast.h"

/**
 * @brief Types of advice supported by the system
 * 
 * ADVICE_BEFORE: Executes before the joinpoint
 * ADVICE_AFTER: Executes after the joinpoint
 * ADVICE_AROUND: Executes around the joinpoint (can control execution)
 */
#define ADVICE_BEFORE 0
#define ADVICE_AFTER  1
#define ADVICE_AROUND 2

/** Weaver debug level (0 = no logs, 1 = basic, 2 = detailed, 3 = very detailed) */
static int debug_level = 0;

/** Weaving process statistics */
static WeavingStats stats = {0};

/**
 * @brief Structure to maintain a list of found aspects
 */
typedef struct {
    AstNode** aspects;  ///< Array of AST nodes representing aspects
    int count;         ///< Number of aspects found
} AspectList;

/** Global list of aspects found during analysis */
static AspectList aspect_list = {NULL, 0};

// Prototipos de funciones internas
static bool collect_aspects(AstNode* ast);
static bool apply_aspects(AstNode* ast);
static bool matches_pointcut(const char* pattern, const char* target);
static AstNode* clone_advice_body(AstNode* advice);
static void insert_advice(AstNode* target, AstNode* advice, int position);

/**
 * @brief Initializes the aspect weaving system
 * 
 * This function:
 * 1. Resets weaver statistics
 * 2. Cleans up the aspect list
 * 3. Prepares the system for a new weaving process
 */
void weaver_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_init);
    
    // Reset statistics
    stats = (WeavingStats){0};
    
    // Clean up aspect list
    if (aspect_list.aspects) {
        free(aspect_list.aspects);
        aspect_list.aspects = NULL;
    }
    aspect_list.count = 0;
    
    logger_log(LOG_INFO, "Aspect weaver initialized");
}

/**
 * @brief Sets the weaver's debug level
 * 
 * @param level Debug level (0-3)
 *             0: No logs
 *             1: Basic logs
 *             2: Detailed logs
 *             3: Very detailed logs
 */
void weaver_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_set_debug_level);
    
    debug_level = level;
    logger_log(LOG_INFO, "Aspect weaver debug level set to %d", level);
}

/**
 * @brief Processes the AST to apply aspects
 * 
 * This function is the main entry point of the weaver. It performs two steps:
 * 1. Collects all aspects defined in the program
 * 2. Applies the found aspects to their corresponding joinpoints
 * 
 * @param ast Root AST node of the program
 * @return true if the process was successful, false otherwise
 */
bool weaver_process(AstNode* ast) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_process);
    
    if (!ast) {
        strncpy(stats.error_msg, "NULL AST provided", sizeof(stats.error_msg)-1);
        return false;
    }
    
    logger_log(LOG_INFO, "Starting aspect weaving process");
    
    // Step 1: Collect all aspects in the program
    if (!collect_aspects(ast)) {
        return false;
    }
    
    if (aspect_list.count == 0) {
        logger_log(LOG_INFO, "No aspects found in the program");
        return true;  // No aspects, but not an error
    }
    
    logger_log(LOG_INFO, "Found %d aspects in the program", aspect_list.count);
    
    // Step 2: Apply the found aspects
    if (!apply_aspects(ast)) {
        return false;
    }
    
    logger_log(LOG_INFO, "Aspect weaving completed: found %d joinpoints, applied %d advice",
              stats.joinpoints_found, stats.advice_applied);
    
    return true;
}

/**
 * @brief Gets the weaving process statistics
 * 
 * @return WeavingStats structure with current statistics
 */
WeavingStats weaver_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_get_stats);
    return stats;
}

/**
 * @brief Cleans up resources used by the weaver
 * 
 * This function frees the memory allocated for the aspect list
 * and prepares the system for a new weaving process
 */
void weaver_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_cleanup);
    
    // Free aspect list memory
    if (aspect_list.aspects) {
        // Don't free AST nodes here as they are part of the main AST
        free(aspect_list.aspects);
        aspect_list.aspects = NULL;
    }
    aspect_list.count = 0;
    
    logger_log(LOG_INFO, "Aspect weaver cleanup completed");
}

/**
 * @brief Collects all aspects defined in the AST
 * 
 * This function recursively traverses the AST looking for aspect
 * definitions and stores them in the global aspect_list.
 * 
 * @param node Current AST node
 * @return true if collection was successful, false otherwise
 */
static bool collect_aspects(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)collect_aspects);
    
    if (!node) return true;
    
    // If we find an aspect, add it to the list
    if (node->type == AST_ASPECT_DEF) {
        aspect_list.count++;
        aspect_list.aspects = memory_realloc(aspect_list.aspects, 
                                           aspect_list.count * sizeof(AstNode*));
        if (!aspect_list.aspects) {
            strncpy(stats.error_msg, "Memory allocation failed", sizeof(stats.error_msg)-1);
            return false;
        }
        aspect_list.aspects[aspect_list.count - 1] = node;
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Collected aspect: %s with %d pointcuts and %d advice", 
                      node->aspectDef.name, node->aspectDef.pointcutCount, node->aspectDef.adviceCount);
        }
    }
    
    // Recursively search in children
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!collect_aspects(node->program.statements[i])) return false;
            }
            break;
            
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (!collect_aspects(node->funcDef.body[i])) return false;
            }
            break;
            
        case AST_IF_STMT:
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                if (!collect_aspects(node->ifStmt.thenBranch[i])) return false;
            }
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                if (!collect_aspects(node->ifStmt.elseBranch[i])) return false;
            }
            break;
            
        case AST_WHILE_STMT:
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                if (!collect_aspects(node->whileStmt.body[i])) return false;
            }
            break;
            
        case AST_DO_WHILE_STMT:
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                if (!collect_aspects(node->doWhileStmt.body[i])) return false;
            }
            break;
            
        case AST_FOR_STMT:
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                if (!collect_aspects(node->forStmt.body[i])) return false;
            }
            break;
            
        // Añadir otros casos según sea necesario
    }
    
    return true;
}

/**
 * @brief Applies the collected aspects to the AST
 * 
 * This function:
 * 1. Finds joinpoints that match the pointcuts
 * 2. Clones and applies the corresponding advice
 * 3. Updates the process statistics
 * 
 * @param node Current AST node
 * @return true if application was successful, false otherwise
 */
static bool apply_aspects(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)apply_aspects);
    
    if (!node) return true;
    
    // If it's a function definition, check if it matches any pointcut
    if (node->type == AST_FUNC_DEF) {
        logger_log(LOG_DEBUG, "Checking function '%s' for aspect application", node->funcDef.name);
        
        for (int i = 0; i < aspect_list.count; i++) {
            AstNode* aspect = aspect_list.aspects[i];
            
            for (int j = 0; j < aspect->aspectDef.pointcutCount; j++) {
                AstNode* pointcut = aspect->aspectDef.pointcuts[j];
                
                logger_log(LOG_DEBUG, "Checking if '%s' matches pattern '%s'", 
                          node->funcDef.name, pointcut->pointcut.pattern);
                
                if (matches_pointcut(pointcut->pointcut.pattern, node->funcDef.name)) {
                    stats.joinpoints_found++;
                    
                    logger_log(LOG_INFO, "Found joinpoint: %s matches %s",
                             node->funcDef.name, pointcut->pointcut.pattern);
                    
                    // Apply all advice associated with this pointcut
                    for (int k = 0; k < aspect->aspectDef.adviceCount; k++) {
                        AstNode* advice = aspect->aspectDef.advice[k];
                        
                        if (strcmp(advice->advice.pointcutName, pointcut->pointcut.name) == 0) {
                            // Clone the advice body
                            AstNode* advice_body = clone_advice_body(advice);
                            if (!advice_body) {
                                strncpy(stats.error_msg, "Failed to clone advice body", sizeof(stats.error_msg)-1);
                                return false;
                            }
                            
                            // Insert advice according to its type
                            switch (advice->advice.type) {
                                case ADVICE_BEFORE:
                                    logger_log(LOG_INFO, "Applying BEFORE advice to %s", node->funcDef.name);
                                    insert_advice(node, advice_body, 0);
                                    break;
                                    
                                case ADVICE_AFTER:
                                    logger_log(LOG_INFO, "Applying AFTER advice to %s", node->funcDef.name);
                                    insert_advice(node, advice_body, -1);
                                    break;
                                    
                                case ADVICE_AROUND:
                                    logger_log(LOG_INFO, "Applying AROUND advice to %s (treating as before)", node->funcDef.name);
                                    insert_advice(node, advice_body, 0);
                                    break;
                                    
                                default:
                                    logger_log(LOG_WARNING, "Unknown advice type: %d", advice->advice.type);
                                    freeAstNode(advice_body);
                                    continue;
                            }
                            
                            stats.advice_applied++;
                            
                            const char* adviceTypeStr;
                            switch (advice->advice.type) {
                                case ADVICE_BEFORE: adviceTypeStr = "before"; break;
                                case ADVICE_AFTER:  adviceTypeStr = "after"; break;
                                case ADVICE_AROUND: adviceTypeStr = "around"; break;
                                default: adviceTypeStr = "unknown"; break;
                            }
                            logger_log(LOG_INFO, "Applied %s advice to %s",
                                      adviceTypeStr,
                                      node->funcDef.name);
                        }
                    }
                }
            }
        }
    }
    
    // Recursively process children
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!apply_aspects(node->program.statements[i])) return false;
            }
            break;
            
        case AST_FUNC_DEF:
            // Process function body after applying aspects
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (!apply_aspects(node->funcDef.body[i])) return false;
            }
            break;
            
        case AST_IF_STMT:
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                if (!apply_aspects(node->ifStmt.thenBranch[i])) return false;
            }
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                if (!apply_aspects(node->ifStmt.elseBranch[i])) return false;
            }
            break;
            
        case AST_WHILE_STMT:
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                if (!apply_aspects(node->whileStmt.body[i])) return false;
            }
            break;
            
        case AST_DO_WHILE_STMT:
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                if (!apply_aspects(node->doWhileStmt.body[i])) return false;
            }
            break;
            
        case AST_FOR_STMT:
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                if (!apply_aspects(node->forStmt.body[i])) return false;
            }
            break;
            
        // Añadir otros casos según sea necesario
    }
    
    return true;
}

/**
 * @brief Checks if a function name matches a pointcut pattern
 * 
 * This function implements a pattern matching system that supports:
 * 1. Exact matching
 * 2. Wildcard at the end (*)
 * 3. Wildcards in the middle of the pattern
 * 
 * @param pattern Pointcut pattern (e.g., "test_*", "get*", "*_test")
 * @param target Function name to check
 * @return true if there's a match, false otherwise
 */
static bool matches_pointcut(const char* pattern, const char* target) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)matches_pointcut);
    
    logger_log(LOG_DEBUG, "Matching '%s' against pattern '%s'", target, pattern);
    
    // Special case for patterns ending with "*" like "test_*"
    size_t pattern_len = strlen(pattern);
    size_t target_len = strlen(target);
    
    if (pattern_len > 0 && pattern[pattern_len - 1] == '*') {
        // Compare characters before the '*'
        size_t prefix_len = pattern_len - 1;
        if (target_len >= prefix_len && strncmp(target, pattern, prefix_len) == 0) {
            logger_log(LOG_INFO, "Match found! Function '%s' matches pattern '%s'", 
                      target, pattern);
            return true;
        }
    }
    
    // Exact match
    if (strcmp(pattern, target) == 0) {
        logger_log(LOG_INFO, "Exact match found! Function '%s' matches pattern '%s'", 
                  target, pattern);
        return true;
    }
    
    // More complex algorithm to handle wildcards in the middle of the pattern
    const char* p = pattern;
    const char* t = target;
    
    while (*p && *t) {
        if (*p == '*') {
            p++;
            if (!*p) return true; // * at the end matches everything
            
            // Look for the next part after the *
            while (*t) {
                if (matches_pointcut(p, t)) return true;
                t++;
            }
            return false;
        }
        else if (*p != *t) {
            return false;
        }
        p++;
        t++;
    }
    
    return *p == *t; // Both must have ended
}

/**
 * @brief Clones the body of an advice
 * 
 * This function creates a deep copy of an advice's body,
 * including all its statements and expressions.
 * 
 * @param advice AST node of the advice to clone
 * @return AstNode* New AST node with the cloned body, NULL on error
 */
static AstNode* clone_advice_body(AstNode* advice) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)clone_advice_body);
    
    if (!advice || advice->type != AST_ADVICE) {
        return NULL;
    }
    
    // Create a block node to contain all advice statements
    AstNode* block = createAstNode(AST_BLOCK);
    if (!block) {
        return NULL;
    }
    
    // Copy each statement from the advice to the block
    block->block.statements = (AstNode**)memory_alloc(sizeof(AstNode*) * advice->advice.bodyCount);
    block->block.statementCount = advice->advice.bodyCount;
    
    for (int i = 0; i < advice->advice.bodyCount; i++) {
        block->block.statements[i] = copyAstNode(advice->advice.body[i]);
        if (!block->block.statements[i]) {
            // If copy fails, free what has been copied so far
            for (int j = 0; j < i; j++) {
                freeAstNode(block->block.statements[j]);
            }
            memory_free(block->block.statements);
            freeAstNode(block);
            return NULL;
        }
    }
    
    return block;
}

/**
 * @brief Inserts an advice into a target function
 * 
 * This function inserts an advice's body at a specific position
 * within a function's body.
 * 
 * @param target AST node of the target function
 * @param advice AST node of the advice to insert
 * @param position Position where to insert the advice (-1 for the end)
 */
static void insert_advice(AstNode* target, AstNode* advice, int position) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)insert_advice);
    
    if (target->type != AST_FUNC_DEF || !advice) return;
    
    // Calculate insertion position
    int insert_pos = (position >= 0) ? position : target->funcDef.bodyCount;
    
    // Make space for the new advice
    target->funcDef.bodyCount++;
    target->funcDef.body = memory_realloc(target->funcDef.body,
                                         target->funcDef.bodyCount * sizeof(AstNode*));
    
    if (!target->funcDef.body) {
        logger_log(LOG_ERROR, "Memory allocation failed when inserting advice");
        return;
    }
    
    // Shift existing elements if necessary
    if (insert_pos < target->funcDef.bodyCount - 1) {
        memmove(&target->funcDef.body[insert_pos + 1],
                &target->funcDef.body[insert_pos],
                (target->funcDef.bodyCount - insert_pos - 1) * sizeof(AstNode*));
    }
    
    // Insert the advice
    target->funcDef.body[insert_pos] = advice;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Inserted advice at position %d in function %s",
                  insert_pos, target->funcDef.name);
    }
}
