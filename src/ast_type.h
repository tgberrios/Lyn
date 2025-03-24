#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "ast.h"
#include "types.h"

// AST type inference and checking functions
void ast_set_type(AstNode* node, Type* type);
Type* ast_infer_type(AstNode* node);
Type* check_binary_op_types(AstNode* node);
bool validate_ast_types(AstNode* node);

#endif // AST_TYPE_H
