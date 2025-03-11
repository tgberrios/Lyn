#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

// Niveles de optimización
#define OPT_LEVEL_0 0  // Sin optimización
#define OPT_LEVEL_1 1  // Optimizaciones básicas (constant folding)
#define OPT_LEVEL_2 2  // Optimizaciones intermedias (+ dead code elimination)
#define OPT_LEVEL_3 3  // Todas las optimizaciones

// Inicializa el optimizador con un nivel específico
void optimizer_init(int level);

// Aplica optimizaciones al AST según el nivel configurado
AstNode* optimize_ast(AstNode* ast);

#endif /* OPTIMIZER_H */
