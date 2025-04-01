/**
 * @file compiler.h
 * @brief Header file for the Lyn compiler that generates C code
 * 
 * This header file defines the interface for the Lyn compiler that translates
 * Lyn source code into C code. It provides functions for compilation control,
 * statistics tracking, and type checking.
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

/**
 * @brief Structure to track compiler statistics during compilation
 * 
 * This structure maintains various counters and metrics about the compilation process,
 * including the number of nodes processed, functions compiled, variables declared,
 * and type errors detected.
 */
typedef struct {
    int nodes_processed;      ///< Number of AST nodes processed during compilation
    int functions_compiled;   ///< Number of functions successfully compiled
    int variables_declared;   ///< Number of variables declared and managed
    int type_errors_detected; ///< Number of type-related errors encountered
} CompilerStats;

/**
 * @brief Sets the debug level for the compiler
 * 
 * Controls the verbosity of debug output during compilation.
 * Higher levels provide more detailed information about the compilation process.
 * 
 * @param level Debug level (0=none, 1=basic, 2=detailed, 3=all)
 */
void compiler_set_debug_level(int level);

/**
 * @brief Retrieves the current compiler statistics
 * 
 * Returns a copy of the current CompilerStats structure containing
 * various metrics about the compilation process.
 * 
 * @return CompilerStats Current compilation statistics
 */
CompilerStats compiler_get_stats(void);

/**
 * @brief Main function to compile Lyn AST to C code
 * 
 * Takes an Abstract Syntax Tree (AST) and generates equivalent C code,
 * writing the output to the specified file path.
 * 
 * @param ast The Abstract Syntax Tree to compile
 * @param outputPath Path where the generated C code should be written
 * @return bool true if compilation was successful, false otherwise
 */
bool compileToC(AstNode* ast, const char* outputPath);

/**
 * @brief Validates type compatibility for variable assignments
 * 
 * Checks if the type of an expression being assigned to a variable
 * is compatible with the variable's declared type.
 * 
 * @param node The AST node representing the variable assignment
 */
void check_assignment_types(AstNode* node);

/**
 * @brief Validates type compatibility for function calls
 * 
 * Checks if the arguments passed to a function call match the
 * expected parameter types of the function.
 * 
 * @param node The AST node representing the function call
 */
void check_function_call_types(AstNode* node);

#endif
