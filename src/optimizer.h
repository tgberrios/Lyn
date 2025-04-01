/**
 * @file optimizer.h
 * @brief Header file for the Lyn compiler's AST optimizer
 * 
 * This header defines the interface for the AST optimization system, which includes:
 * - Multiple optimization levels (0-2)
 * - Various optimization passes (constant folding, dead code elimination, etc.)
 * - Statistics tracking for optimizations
 * - Configurable optimization options
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"
#include "error.h"
#include "logger.h"

/**
 * @brief Optimization levels available in the compiler
 * 
 * Defines different levels of optimization that can be applied to the AST:
 * - OPT_LEVEL_0: No optimization, AST is left unchanged
 * - OPT_LEVEL_1: Basic optimizations (constant folding, redundant statement removal)
 * - OPT_LEVEL_2: Advanced optimizations (dead code elimination, constant propagation)
 */
typedef enum {
    OPT_LEVEL_0 = 0,  ///< No optimization
    OPT_LEVEL_1 = 1,  ///< Basic optimizations
    OPT_LEVEL_2 = 2   ///< Advanced optimizations
} OptimizerLevel;

/**
 * @brief Statistics tracking structure for optimization passes
 * 
 * Keeps track of various optimization metrics:
 * - Number of constant folding operations performed
 * - Amount of dead code removed
 * - Number of redundant assignments eliminated
 * - Count of constant propagations
 * - Number of common subexpressions eliminated
 * - Variables properly scoped
 * - Total number of optimizations applied
 */
typedef struct {
    int constant_folding_applied;      ///< Number of constant folding operations
    int dead_code_removed;             ///< Number of dead code blocks removed
    int redundant_assignments_removed; ///< Number of redundant assignments eliminated
    int constants_propagated;          ///< Number of constant propagations performed
    int cse_eliminated;                ///< Number of common subexpressions eliminated
    int variables_scoped;              ///< Number of variables with proper scope analysis
    int total_optimizations;           ///< Total number of optimizations applied
} OptimizationStats;

/**
 * @brief Configuration options for the optimizer
 * 
 * Allows enabling/disabling specific optimization passes:
 * - Constant folding
 * - Dead code elimination
 * - Redundant statement removal
 * - Constant propagation
 * - Common subexpression elimination
 * - Scope analysis
 */
typedef struct {
    bool enable_constant_folding;           ///< Enable constant folding optimization
    bool enable_dead_code_elimination;      ///< Enable dead code elimination
    bool enable_redundant_stmt_removal;     ///< Enable redundant statement removal
    bool enable_constant_propagation;       ///< Enable constant propagation
    bool enable_common_subexpr_elimination; ///< Enable common subexpression elimination
    bool enable_scope_analysis;             ///< Enable scope analysis
} OptimizerOptions;

/**
 * @brief Initializes the optimizer with a specified optimization level
 * 
 * Sets up the optimizer with the given optimization level and resets
 * optimization statistics.
 * 
 * @param level Optimization level to use (0-2)
 */
void optimizer_init(OptimizerLevel level);

/**
 * @brief Sets the debug level for the optimizer
 * 
 * Controls the verbosity of optimization-related logging:
 * - 0: Minimal logging
 * - 1: Basic optimization info
 * - 2: Detailed optimization info
 * - 3: Verbose debugging output
 * 
 * @param level New debug level (0-3)
 */
void optimizer_set_debug_level(int level);

/**
 * @brief Gets the current debug level for the optimizer
 * 
 * @return int Current debug level (0-3)
 */
int optimizer_get_debug_level(void);

/**
 * @brief Optimizes the Abstract Syntax Tree
 * 
 * Applies various optimization passes to the AST based on the current
 * optimization level and enabled options.
 * 
 * @param ast AST to optimize
 * @return AstNode* Optimized AST, or NULL if input is NULL
 */
AstNode* optimize_ast(AstNode* ast);

/**
 * @brief Gets the current optimization statistics
 * 
 * Returns statistics about optimizations performed during the last
 * optimization pass.
 * 
 * @return OptimizationStats Current optimization statistics
 */
OptimizationStats optimizer_get_stats(void);

/**
 * @brief Sets the optimizer options
 * 
 * Configures which optimization passes should be enabled or disabled.
 * 
 * @param options New optimizer options to apply
 */
void optimizer_set_options(OptimizerOptions options);

/**
 * @brief Gets the current optimizer options
 * 
 * @return OptimizerOptions Current optimizer options
 */
OptimizerOptions optimizer_get_options(void);

#endif // OPTIMIZER_H
