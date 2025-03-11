#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <stdbool.h>

// Compila el AST proporcionado a código C
bool compileToC(AstNode* ast, const char* outputPath);

#endif /* COMPILER_H */
