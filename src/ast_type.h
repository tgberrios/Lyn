#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "ast.h"
#include "types.h"

/**
 * @file ast_type.h
 * @brief Header file for AST type inference and checking functionality
 * 
 * This header file provides the interface for type inference and type checking
 * operations on Abstract Syntax Tree (AST) nodes. It includes functions for
 * assigning types to nodes, inferring types from expressions, and validating
 * type compatibility across the AST.
 */

/**
 * @brief Assigns a type to an AST node
 * 
 * This function sets the inferred type for a given AST node. It is used to
 * store type information that has been determined through type inference or
 * explicit type declarations.
 * 
 * @param node The AST node to set the type for
 * @param type The type to assign to the node
 */
void ast_set_type(AstNode* node, Type* type);

/**
 * @brief Infers the type of an AST node
 * 
 * This function performs type inference on an AST node, determining its type
 * based on the node's content and context. It handles various node types including
 * literals, binary operations, function calls, and identifiers.
 * 
 * @param node The AST node to infer the type for
 * @return Type* The inferred type of the node, or TYPE_UNKNOWN if type cannot be determined
 */
Type* ast_infer_type(AstNode* node);

/**
 * @brief Performs type checking for binary operations
 * 
 * This function specifically handles type checking for binary operations,
 * ensuring type compatibility between operands and determining the result type.
 * It supports arithmetic, comparison, and logical operations.
 * 
 * @param node The binary operation AST node to check
 * @return Type* The inferred type of the operation result
 */
Type* check_binary_op_types(AstNode* node);

/**
 * @brief Validates types across the entire AST
 * 
 * This function performs a comprehensive type check on the AST, ensuring
 * type compatibility across all operations and assignments. It recursively
 * checks all nodes and their children for type errors.
 * 
 * @param node The root AST node to validate
 * @return bool true if all types are valid, false if any type errors are found
 */
bool validate_ast_types(AstNode* node);

#endif // AST_TYPE_H
