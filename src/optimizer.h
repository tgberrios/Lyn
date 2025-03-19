#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"
#include "error.h"
#include "logger.h"

// Define optimization levels
typedef enum {
    OPT_LEVEL_0 = 0,  // No optimization
    OPT_LEVEL_1 = 1,  // Basic optimizations
    OPT_LEVEL_2 = 2   // Advanced optimizations
} OptimizerLevel;

// Initialize the optimizer with the given level
void optimizer_init(OptimizerLevel level);

// Set debug level for optimizer (0=minimal, 3=verbose)
void optimizer_set_debug_level(int level);

// Get current debug level
int optimizer_get_debug_level(void);

// Optimize the AST
AstNode* optimize_ast(AstNode* ast);

// Get optimization statistics
typedef struct {
    int constant_folding_applied;
    int dead_code_removed;
    int redundant_assignments_removed;
    int total_optimizations;
} OptimizationStats;

OptimizationStats optimizer_get_stats(void);

#endif // OPTIMIZER_H
