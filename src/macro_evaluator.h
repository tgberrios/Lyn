/**
 * @file macro_evaluator.h
 * @brief Header file for the macro system in the Lyn compiler
 * 
 * This header defines the interface for the macro system, which provides
 * compile-time code generation and manipulation capabilities. It includes
 * functions for macro definition, expansion, and various macro operations.
 */

#ifndef MACRO_EVALUATOR_H
#define MACRO_EVALUATOR_H

#include "ast.h"
#include <stdbool.h>

/**
 * @brief Initializes the macro system
 * 
 * Prepares the macro system for use by resetting internal state and
 * allocating necessary resources.
 */
void macro_init(void);

/**
 * @brief Sets the debug level for the macro system
 * 
 * Controls the verbosity of debug output from the macro system.
 * Higher levels provide more detailed information about macro operations.
 * 
 * @param level The new debug level (0=none, 1=basic, 2=detailed, 3=all)
 */
void macro_set_debug_level(int level);

/**
 * @brief Registers a new macro definition
 * 
 * Converts a function definition into a macro and registers it in the
 * macro system. The function should be marked as a macro using the
 * "macro_" prefix convention.
 * 
 * @param node The AST node containing the function definition to convert
 * @return bool true if registration was successful, false otherwise
 */
bool register_macro(AstNode* node);

/**
 * @brief Expands a macro call with the given arguments
 * 
 * Creates a new AST by substituting the macro's parameters with the
 * provided arguments and expanding the macro's body.
 * 
 * @param name The name of the macro to expand
 * @param args Array of argument AST nodes
 * @param argCount Number of arguments
 * @return AstNode* The expanded macro as an AST node, or NULL if expansion failed
 */
AstNode* expand_macro(const char* name, AstNode** args, int argCount);

/**
 * @brief Converts an AST node to its string representation
 * 
 * Implements the # operator functionality, converting an AST node into
 * its string representation. This is useful for debugging and
 * compile-time string manipulation.
 * 
 * @param node The AST node to stringify
 * @return char* String representation of the node, or "NULL" if node is NULL
 */
char* macro_stringify(AstNode* node);

/**
 * @brief Concatenates two strings
 * 
 * Implements the ## operator functionality, joining two strings together
 * at compile time. This is useful for token pasting operations.
 * 
 * @param s1 First string
 * @param s2 Second string
 * @return char* Concatenated string, or empty string if either input is NULL
 */
char* macro_concat(const char* s1, const char* s2);

/**
 * @brief Processes the entire AST to evaluate all macros
 * 
 * Recursively traverses the AST and expands any macro calls found.
 * This function handles both macro definitions and macro expansions,
 * ensuring that all macros are properly processed.
 * 
 * @param node The AST node to process
 * @return AstNode* The processed AST node, or NULL if the node should be removed
 */
AstNode* evaluate_macros(AstNode* node);

/**
 * @brief Cleans up resources used by the macro system
 * 
 * Frees all memory allocated for macro definitions and resets the
 * system to its initial state. This should be called when the
 * macro system is no longer needed.
 */
void macro_cleanup(void);

#endif /* MACRO_EVALUATOR_H */
