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
    int constants_propagated;    // New counter for constant propagation
    int cse_eliminated;          // New counter for common subexpressions eliminated
    int variables_scoped;        // New counter for variables with proper scope analysis
    int total_optimizations;
} OptimizationStats;

OptimizationStats optimizer_get_stats(void);

// New function to enable or disable specific optimizations
typedef struct {
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_redundant_stmt_removal;
    bool enable_constant_propagation;    // New optimization
    bool enable_common_subexpr_elimination;  // New optimization
    bool enable_scope_analysis;         // New optimization
} OptimizerOptions;

// Set optimizer options
void optimizer_set_options(OptimizerOptions options);

// Get current optimizer options
OptimizerOptions optimizer_get_options(void);

#endif // OPTIMIZER_H
