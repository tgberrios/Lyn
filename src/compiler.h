#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

// Compiler statistics
typedef struct {
    int nodes_processed;
    int functions_compiled;
    int variables_declared;
    int type_errors_detected;  // Added to track type errors
} CompilerStats;

// Function to set debug level (0=none, 1=basic, 2=detailed, 3=all)
void compiler_set_debug_level(int level);

// Function to get compiler statistics
CompilerStats compiler_get_stats(void);

// Main compilation function
bool compileToC(AstNode* ast, const char* outputPath);

// Type checking functions - accessible for testing
void check_assignment_types(AstNode* node);
void check_function_call_types(AstNode* node);

#endif
