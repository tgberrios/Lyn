#ifndef LYN_OPTIMIZER_H
#define LYN_OPTIMIZER_H

#include "ast.h"
#include <stdbool.h>

typedef enum {
    OPT_LEVEL_0,  // Sin optimizaciones
    OPT_LEVEL_1,  // Optimizaciones básicas
    OPT_LEVEL_2,  // Optimizaciones agresivas
    OPT_LEVEL_3   // Todas las optimizaciones
} OptimizationLevel;

void optimizer_init(OptimizationLevel level);
AstNode* optimize_ast(AstNode* ast);

#endif // LYN_OPTIMIZER_H
