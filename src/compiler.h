#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

// Compila el AST proporcionado a código C
bool compileToC(AstNode* ast, const char* outputPath);

// Establece el nivel de depuración para el compilador
void compiler_set_debug_level(int level);

// Obtiene estadísticas de compilación
typedef struct {
    int nodes_processed;
    int functions_compiled;
    int variables_declared;
    int errors_encountered;
} CompilerStats;

// Obtiene estadísticas de la última compilación
CompilerStats compiler_get_stats(void);

#endif /* COMPILER_H */
