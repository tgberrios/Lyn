#ifndef MACRO_EVALUATOR_H
#define MACRO_EVALUATOR_H

#include "ast.h"

// Evaluates macros in an AST at compile time
AstNode* evaluate_macros(AstNode* node);

// Stores a macro definition for later use
bool register_macro(AstNode* macroDef);

// Expands a macro with given arguments
AstNode* expand_macro(const char* name, AstNode** args, int argCount);

// String operations for macros
char* macro_stringify(AstNode* node);
char* macro_concat(const char* s1, const char* s2);

#endif
