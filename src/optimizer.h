#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

// Define optimization levels
typedef enum {
    OPT_LEVEL_0 = 0,  // No optimization
    OPT_LEVEL_1 = 1,  // Basic optimizations
    OPT_LEVEL_2 = 2   // Advanced optimizations
} OptimizerLevel;

// Initialize the optimizer with the given level
void optimizer_init(OptimizerLevel level);

// Optimize the AST
AstNode* optimize_ast(AstNode* ast);

#endif // OPTIMIZER_H
