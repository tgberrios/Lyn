#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

/**
 * @brief Compila el AST a código C
 * 
 * @param ast Raíz del AST a compilar
 * @param outputPath Ruta del archivo de salida
 * @return true si la compilación fue exitosa
 * @return false si hubo algún error
 */
bool compileToC(AstNode *ast, const char *outputPath);

#endif /* COMPILER_H */
