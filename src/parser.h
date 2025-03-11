#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/**
 * @brief Parsea el código fuente y devuelve la raíz del AST.
 *
 * @return AstNode* Puntero a la raíz del AST.
 */
AstNode *parseProgram(void);

/**
 * @brief Libera el AST generado.
 *
 * @param root Puntero a la raíz del AST.
 */
void freeAst(AstNode *root);

#endif /* PARSER_H */
