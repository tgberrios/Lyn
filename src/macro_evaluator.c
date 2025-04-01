/**
 * @file macro_evaluator.c
 * @brief Implementation of the macro system for the Lyn compiler
 * 
 * This file implements a macro system that allows for compile-time code generation
 * and manipulation. It supports macro definitions, expansions, and various macro
 * operations like stringification and concatenation.
 */

#include <stdio.h>
#include <stdlib.h>
#include "macro_evaluator.h"
#include "error.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

// Additional AST node types for macro support
#define AST_MACRO_DEF 100    ///< Macro definition node type
#define AST_MACRO_PARAM 101  ///< Macro parameter node type
#define AST_MACRO_EXPAND 102 ///< Macro expansion node type

#define MAX_MACROS 1024      ///< Maximum number of macros that can be defined

static int debug_level = 1;  ///< Current debug level for the macro system

/**
 * @brief Structure to store macro definitions
 */
typedef struct {
    char name[256];          ///< Name of the macro
    char** params;           ///< Array of parameter names
    int paramCount;          ///< Number of parameters
    AstNode** body;          ///< Array of AST nodes in the macro body
    int bodyCount;           ///< Number of nodes in the body
} MacroDef;

static MacroDef macros[MAX_MACROS];  ///< Array of defined macros
static int macroCount = 0;           ///< Current number of defined macros

/**
 * @brief Sets the debug level for the macro system
 * 
 * @param level The new debug level (0=none, 1=basic, 2=detailed, 3=all)
 */
void macro_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Macro system debug level set to %d", level);
}

/**
 * @brief Registers a new macro definition
 * 
 * Registers a macro definition from an AST node. The node should be either
 * a function definition (AST_FUNC_DEF) or a macro definition (AST_MACRO_DEF).
 * 
 * @param node The AST node containing the macro definition
 * @return bool true if registration was successful, false otherwise
 */
bool register_macro(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)register_macro);
    
    // Basic verification
    if (!node || macroCount >= MAX_MACROS) {
        logger_log(LOG_WARNING, "Failed to register macro: %s", 
                  !node ? "NULL node" : "Too many macros");
        return false;
    }

    // For compatibility with the rest of the system, first check if
    // the node is a function definition (AST_FUNC_DEF) and treat it
    // as a macro definition
    if (node->type == AST_FUNC_DEF) {
        MacroDef* macro = &macros[macroCount++];
        strncpy(macro->name, node->funcDef.name, sizeof(macro->name) - 1);
        
        if (debug_level >= 1) {
            logger_log(LOG_INFO, "Registering macro: %s with %d parameters",
                     macro->name, node->funcDef.paramCount);
        }
        
        // Copy parameters
        macro->paramCount = node->funcDef.paramCount;
        macro->params = malloc(macro->paramCount * sizeof(char*));
        if (!macro->params) {
            logger_log(LOG_ERROR, "Memory allocation failed for macro parameters");
            return false;
        }
        
        for (int i = 0; i < macro->paramCount; i++) {
            if (node->funcDef.parameters[i]->type == AST_IDENTIFIER) {
                macro->params[i] = strdup(node->funcDef.parameters[i]->identifier.name);
            } else {
                macro->params[i] = strdup("unknown");
            }
        }
        
        // Copy body
        macro->bodyCount = node->funcDef.bodyCount;
        macro->body = malloc(macro->bodyCount * sizeof(AstNode*));
        if (!macro->body) {
            logger_log(LOG_ERROR, "Memory allocation failed for macro body");
            // Free parameter memory
            for (int i = 0; i < macro->paramCount; i++) {
                free(macro->params[i]);
            }
            free(macro->params);
            return false;
        }
        
        for (int i = 0; i < macro->bodyCount; i++) {
            macro->body[i] = node->funcDef.body[i];
        }
        
        return true;
    }
    
    // If not a function, it could be a specific AST_MACRO_DEF node
    // (this is to maintain compatibility with code using AST_MACRO_DEF)
    if (node->type == AST_MACRO_DEF) {
        logger_log(LOG_WARNING, "AST_MACRO_DEF is deprecated, use AST_FUNC_DEF instead");
        return false;  // Currently not supported
    }
    
    logger_log(LOG_WARNING, "Node type %d is not supported for macro registration", node->type);
    return false;
}

/**
 * @brief Finds a macro definition by name
 * 
 * @param name The name of the macro to find
 * @return MacroDef* Pointer to the macro definition if found, NULL otherwise
 */
static MacroDef* find_macro(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)find_macro);
    
    for (int i = 0; i < macroCount; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return &macros[i];
        }
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Macro not found: %s", name);
    }
    
    return NULL;
}

/**
 * @brief Expands a macro with the given arguments
 * 
 * Creates a new AST by substituting the macro's parameters with the provided
 * arguments and expanding the macro's body.
 * 
 * @param name The name of the macro to expand
 * @param args Array of argument AST nodes
 * @param argCount Number of arguments
 * @return AstNode* The expanded macro as an AST node, or NULL if expansion failed
 */
AstNode* expand_macro(const char* name, AstNode** args, int argCount) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)expand_macro);
    
    MacroDef* macro = find_macro(name);
    if (!macro) {
        logger_log(LOG_WARNING, "Attempted to expand undefined macro: %s", name);
        return NULL;
    }
    
    if (macro->paramCount != argCount) {
        logger_log(LOG_WARNING, "Macro %s expects %d arguments, but %d were provided",
                  name, macro->paramCount, argCount);
        return NULL;
    }
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "Expanding macro: %s with %d arguments", name, argCount);
    }
    
    // Create new scope for macro expansion
    // Map parameters to arguments
    AstNode** expanded = malloc(macro->bodyCount * sizeof(AstNode*));
    if (!expanded) {
        logger_log(LOG_ERROR, "Memory allocation failed for macro expansion");
        return NULL;
    }
    
    // Simple parameter substitution strategy
    for (int i = 0; i < macro->bodyCount; i++) {
        // Create a deep copy of each statement to avoid modifying the original
        expanded[i] = copyAstNode(macro->body[i]);
        
        // For real substitution, we would need to traverse the AST and replace
        // all occurrences of parameter names with the corresponding arguments
        // This is a simplified version
    }
    
    // Create block node with expanded macro body
    AstNode* block = createAstNode(AST_PROGRAM);
    if (!block) {
        logger_log(LOG_ERROR, "Failed to create block node for macro expansion");
        for (int i = 0; i < macro->bodyCount; i++) {
            freeAstNode(expanded[i]);
        }
        free(expanded);
        return NULL;
    }
    
    block->program.statements = expanded;
    block->program.statementCount = macro->bodyCount;
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "Macro %s expanded to %d statements", name, macro->bodyCount);
    }
    
    return block;
}

/**
 * @brief Converts an AST node to its string representation
 * 
 * @param node The AST node to stringify
 * @return char* String representation of the node, or "NULL" if node is NULL
 */
char* macro_stringify(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_stringify);
    
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to stringify NULL node");
        return strdup("NULL");
    }
    
    char buffer[1024];
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            snprintf(buffer, sizeof(buffer), "%g", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            snprintf(buffer, sizeof(buffer), "\"%s\"", node->stringLiteral.value);
            break;
        case AST_IDENTIFIER:
            snprintf(buffer, sizeof(buffer), "%s", node->identifier.name);
            break;
        case AST_BOOLEAN_LITERAL:
            snprintf(buffer, sizeof(buffer), "%s", node->boolLiteral.value ? "true" : "false");
            break;
        default:
            return strdup("<<unprintable>>");
    }
    
    return strdup(buffer);
}

/**
 * @brief Concatenates two strings
 * 
 * @param s1 First string
 * @param s2 Second string
 * @return char* Concatenated string, or empty string if either input is NULL
 */
char* macro_concat(const char* s1, const char* s2) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_concat);
    
    if (!s1 || !s2) {
        logger_log(LOG_WARNING, "Attempted to concat with NULL string");
        return strdup(s1 ? s1 : (s2 ? s2 : ""));
    }
    
    size_t len = strlen(s1) + strlen(s2) + 1;
    char* result = malloc(len);
    if (!result) {
        logger_log(LOG_ERROR, "Memory allocation failed for string concatenation");
        return NULL;
    }
    
    snprintf(result, len, "%s%s", s1, s2);
    return result;
}

/**
 * @brief Evaluates and expands macros in an AST
 * 
 * Recursively traverses the AST and expands any macro calls found.
 * This function handles both macro definitions and macro expansions.
 * 
 * @param node The AST node to process
 * @return AstNode* The processed AST node, or NULL if the node should be removed
 */
AstNode* evaluate_macros(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)evaluate_macros);
    
    if (!node) return NULL;

    // In our current AST we use AST_FUNC_DEF instead of AST_MACRO_DEF
    // We can recognize macros by convention, for example, by a prefix in the name
    if (node->type == AST_FUNC_DEF) {
        // If the name starts with "macro_", we consider it a macro
        if (strncmp(node->funcDef.name, "macro_", 6) == 0) {
            if (register_macro(node)) {
                // Return NULL to remove the definition from the final AST
                // since it has been registered as a macro
                return NULL;
            }
        }
    }
    
    // For compatibility with original code
    if (node->type == AST_MACRO_DEF) {
        if (register_macro(node)) {
            return NULL; // Remove macro definition from AST
        }
    }
    
    // For compatibility with original code
    if (node->type == AST_MACRO_EXPAND) {
        // This would be the original case, but for our current AST
        // we would use a function call with special name
        return NULL;
    }
    
    // Detect macro calls by function name
    if (node->type == AST_FUNC_CALL) {
        MacroDef* macro = find_macro(node->funcCall.name);
        if (macro) {
            // It's a call to a registered macro
            return expand_macro(node->funcCall.name, 
                               node->funcCall.arguments, 
                               node->funcCall.argCount);
        }
    }
    
    // Recursively process children
    switch (node->type) {
        case AST_PROGRAM:
            // Process each statement and replace if necessary
            {
                int newCount = 0;
                AstNode** newStatements = malloc(node->program.statementCount * sizeof(AstNode*));
                if (!newStatements) {
                    logger_log(LOG_ERROR, "Memory allocation failed for macro processing");
                    return node;
                }
                
                for (int i = 0; i < node->program.statementCount; i++) {
                    AstNode* result = evaluate_macros(node->program.statements[i]);
                    if (result) {
                        // If the result is a program (macro expansion),
                        // incorporate its statements directly
                        if (result->type == AST_PROGRAM) {
                            for (int j = 0; j < result->program.statementCount; j++) {
                                newStatements[newCount++] = result->program.statements[j];
                            }
                            // Free the program node but not its statements
                            free(result->program.statements);
                            free(result);
                        } else {
                            newStatements[newCount++] = result;
                        }
                    }
                }
                
                // Free the original array but not its elements
                free(node->program.statements);
                node->program.statements = newStatements;
                node->program.statementCount = newCount;
            }
            break;
            
        case AST_FUNC_DEF:
            // Process function body
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                AstNode* result = evaluate_macros(node->funcDef.body[i]);
                if (result) {
                    node->funcDef.body[i] = result;
                }
            }
            break;
            
        case AST_IF_STMT:
            // Process condition
            {
                AstNode* result = evaluate_macros(node->ifStmt.condition);
                if (result) {
                    node->ifStmt.condition = result;
                }
            }
            
            // Process 'then' branch
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                AstNode* result = evaluate_macros(node->ifStmt.thenBranch[i]);
                if (result) {
                    node->ifStmt.thenBranch[i] = result;
                }
            }
            
            // Process 'else' branch
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                AstNode* result = evaluate_macros(node->ifStmt.elseBranch[i]);
                if (result) {
                    node->ifStmt.elseBranch[i] = result;
                }
            }
            break;
            
        default:
            // For other node types that don't contain children, do nothing
            break;
    }
    
    return node;
}

/**
 * @brief Initializes the macro system
 * 
 * Resets the macro counter and prepares the system for use.
 */
void macro_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_init);
    
    // Reset macro counter
    macroCount = 0;
    
    logger_log(LOG_INFO, "Macro system initialized");
}

/**
 * @brief Cleans up the macro system
 * 
 * Frees all memory allocated for macro definitions and resets the system.
 */
void macro_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_cleanup);
    
    // Free memory for all macros
    for (int i = 0; i < macroCount; i++) {
        for (int j = 0; j < macros[i].paramCount; j++) {
            free(macros[i].params[j]);
        }
        free(macros[i].params);
        
        // We don't free macro bodies because they are references
        // to nodes that belong to the main AST
        free(macros[i].body);
    }
    
    macroCount = 0;
    
    logger_log(LOG_INFO, "Macro system cleanup complete");
}
