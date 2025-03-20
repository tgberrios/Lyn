#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "error.h"
#include "logger.h"

/**
 * @brief Parsea el código fuente y devuelve la raíz del AST.
 *
 * @return AstNode* Puntero a la raíz del AST.
 */
AstNode *parseProgram(void);

/**
 * @brief Establece el nivel de depuración para el parser.
 * 
 * @param level Nivel de detalle (0=mínimo, 3=máximo).
 */
void parser_set_debug_level(int level);

/**
 * @brief Obtiene el nivel actual de depuración del parser.
 * 
 * @return int Nivel de depuración actual.
 */
int parser_get_debug_level(void);

/**
 * @brief Obtiene estadísticas del parser como número de nodos procesados.
 * 
 * @param nodes_created Puntero donde se almacenará el número de nodos creados.
 * @param errors_found Puntero donde se almacenará el número de errores encontrados.
 */
void parser_get_stats(int* nodes_created, int* errors_found);

/**
 * @brief Libera el AST generado.
 *
 * @param root Puntero a la raíz del AST.
 */
void freeAst(AstNode *root);

// Prototipos de funciones auxiliares internas
void nextToken(void);
void expectToken(int tokenType);
AstNode** parseBlock(int* count);

#endif /* PARSER_H */
