#ifndef MACRO_EVALUATOR_H
#define MACRO_EVALUATOR_H

#include "ast.h"
#include <stdbool.h>

// Inicializa el sistema de macros
void macro_init(void);

// Establece el nivel de depuración
void macro_set_debug_level(int level);

// Registra una macro (función convertida en macro)
bool register_macro(AstNode* node);

// Expande una llamada a macro
AstNode* expand_macro(const char* name, AstNode** args, int argCount);

// Convierte un nodo AST a su representación en cadena para operador #
char* macro_stringify(AstNode* node);

// Concatena dos cadenas para operador ##
char* macro_concat(const char* s1, const char* s2);

// Procesa el AST completo para evaluar todas las macros
AstNode* evaluate_macros(AstNode* node);

// Limpia recursos del sistema de macros
void macro_cleanup(void);

#endif /* MACRO_EVALUATOR_H */
