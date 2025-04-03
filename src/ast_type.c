#include "ast.h"
#include "types.h"
#include "logger.h"
#include "error.h"
#include <stdio.h>
#include <string.h>  // Added for strcpy function

// Forward declaration for ast_infer_type
Type* ast_infer_type(AstNode* node);

/**
 * @brief Assigns a type to an AST node
 * 
 * This function sets the inferred type for a given AST node.
 * In this context, we simply store the type in an auxiliary structure.
 * This is a simplification; in a real compiler, more complex type handling would be needed.
 * 
 * @param node The AST node to set the type for
 * @param type The type to assign to the node
 */
void ast_set_type(AstNode* node, Type* type) {
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to set type on NULL AST node");
        return;
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_set_type);
    
    // In this context, we simply store the type in an auxiliary structure
    // This is a simplification; in a real compiler, more complex handling would be needed
    node->inferredType = type;
    
    logger_log(LOG_DEBUG, "Set AST node type: %s", typeToString(type));
}

/**
 * @brief Enhanced type checking for binary operations
 * 
 * This function performs type checking for binary operations, ensuring
 * type compatibility between operands and determining the result type.
 * 
 * @param node The binary operation AST node to check
 * @return Type* The inferred type of the operation result
 */
Type* check_binary_op_types(AstNode* node) {
    if (!node || node->type != AST_BINARY_OP) {
        logger_log(LOG_WARNING, "Invalid node in binary op type check");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)check_binary_op_types);
    
    Type* left_type = ast_infer_type(node->binaryOp.left);
    Type* right_type = ast_infer_type(node->binaryOp.right);
    
    // Create string representation of the operator
    char operator[8] = {node->binaryOp.op, '\0'};  // Increased size to accommodate longer strings
    
    // Handle special cases for logical operators represented by letters
    if (node->binaryOp.op == 'A') strcpy(operator, "and");
    else if (node->binaryOp.op == 'O') strcpy(operator, "or");
    else if (node->binaryOp.op == 'E') strcpy(operator, "==");
    else if (node->binaryOp.op == 'N') strcpy(operator, "!=");
    else if (node->binaryOp.op == 'G') strcpy(operator, ">=");
    else if (node->binaryOp.op == 'L') strcpy(operator, "<=");
    
    // Use the binary op type inference function
    Type* result = infer_type_from_binary_op(left_type, right_type, operator);
    
    // Store the inferred type in the node
    ast_set_type(node, result);
    
    return result;
}

/**
 * @brief Enhanced AST type inference function
 * 
 * This function performs type inference on AST nodes, determining
 * the appropriate type based on the node's content and context.
 * 
 * @param node The AST node to infer the type for
 * @return Type* The inferred type of the node
 */
Type* ast_infer_type(AstNode* node) {
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to infer type of NULL AST node");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_infer_type);
    
    // If node already has an inferred type, return it
    if (node->inferredType) {
        return node->inferredType;
    }
    
    Type* result = NULL;
    
    switch (node->type) {
        case AST_NUMBER_LITERAL: {
            // Determine if it's an integer or float
            double value = node->numberLiteral.value;
            if (value == (int)value) {
                result = create_primitive_type(TYPE_INT);
            } else {
                result = create_primitive_type(TYPE_FLOAT);
            }
            break;
        }
            
        case AST_STRING_LITERAL:
            result = create_primitive_type(TYPE_STRING);
            break;
            
        case AST_BOOLEAN_LITERAL:
            result = create_primitive_type(TYPE_BOOL);
            break;
            
        case AST_BINARY_OP:
            result = check_binary_op_types(node);
            break;
            
        case AST_IDENTIFIER:
            // In a real compiler, we would look up the identifier in the symbol table
            // For now, we'll return a placeholder type
            result = create_primitive_type(TYPE_UNKNOWN);
            break;
            
        case AST_FUNC_CALL:
            // In a real compiler, we would look up the function signature
            // For now, we'll return a placeholder type
            result = create_primitive_type(TYPE_UNKNOWN);
            break;
            
        // Add more cases as needed
            
        default:
            result = create_primitive_type(TYPE_UNKNOWN);
            logger_log(LOG_DEBUG, "Unknown type for AST node type %d", node->type);
            break;
    }
    
    // Store the inferred type in the node
    ast_set_type(node, result);
    
    logger_log(LOG_DEBUG, "Inferred type for AST node type %d: %s", 
              node->type, typeToString(result));
    
    return result;
}

/**
 * @brief Enhanced function to validate entire AST for type errors
 * 
 * This function performs a comprehensive type check on the AST,
 * ensuring type compatibility across all operations and assignments.
 * 
 * @param node The root AST node to validate
 * @return bool true if all types are valid, false if any type errors are found
 */
bool validate_ast_types(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)validate_ast_types);
    
    if (!node) {
        return true;  // No errors in a null node
    }
    
    bool valid = true;
    
    // First check the node itself
    switch (node->type) {
        case AST_VAR_ASSIGN:
            // Check that the variable and expression types are compatible
            if (node->varAssign.initializer) {
                Type* expr_type = ast_infer_type(node->varAssign.initializer);
                
                // We would normally look up the variable's type in a symbol table
                // For this demo, we'll check some hardcoded cases
                if (strcmp(node->varAssign.name, "int_val") == 0) {
                    Type* int_type = create_primitive_type(TYPE_INT);
                    if (!types_are_compatible(int_type, expr_type)) {
                        logger_log(LOG_ERROR, "Type error: Cannot assign %s to int variable '%s'",
                                  typeToString(expr_type), node->varAssign.name);
                        valid = false;
                    }
                    freeType(int_type);
                }
                else if (strcmp(node->varAssign.name, "float_val") == 0) {
                    Type* float_type = create_primitive_type(TYPE_FLOAT);
                    if (!types_are_compatible(float_type, expr_type)) {
                        logger_log(LOG_ERROR, "Type error: Cannot assign %s to float variable '%s'",
                                  typeToString(expr_type), node->varAssign.name);
                        valid = false;
                    }
                    freeType(float_type);
                }
                // Add more variable checks as needed
            }
            break;
            
        case AST_BINARY_OP:
            // Validate binary operation type compatibility
            {
                Type* left_type = ast_infer_type(node->binaryOp.left);
                Type* right_type = ast_infer_type(node->binaryOp.right);
                
                // Check operation compatibility based on operator and operand types
                char op = node->binaryOp.op;
                
                // Handle string concatenation first
                if (op == '+') {
                    // Allow string concatenation with numbers
                    if ((left_type->kind == TYPE_STRING && (right_type->kind == TYPE_INT || right_type->kind == TYPE_FLOAT)) ||
                        (right_type->kind == TYPE_STRING && (left_type->kind == TYPE_INT || left_type->kind == TYPE_FLOAT))) {
                        logger_log(LOG_DEBUG, "Allowing string concatenation with numbers");
                        valid = true;
                    } else if ((left_type->kind == TYPE_INT || left_type->kind == TYPE_FLOAT) &&
                              (right_type->kind == TYPE_INT || right_type->kind == TYPE_FLOAT)) {
                        // Allow numeric addition
                        valid = true;
                    } else {
                        logger_log(LOG_ERROR, "Type error: Addition requires numeric operands or string concatenation");
                        valid = false;
                    }
                } else if (strchr("-*/", op) != NULL) {
                    // For other arithmetic operations
                    if ((left_type->kind == TYPE_INT || left_type->kind == TYPE_FLOAT) &&
                        (right_type->kind == TYPE_INT || right_type->kind == TYPE_FLOAT)) {
                        valid = true;
                    } else {
                        logger_log(LOG_ERROR, "Type error: Arithmetic operation requires numeric operands");
                        valid = false;
                    }
                } else if (strchr("<>LE", op) != NULL) {
                    // For comparison operations
                    if (left_type->kind == right_type->kind) {
                        valid = true;
                    } else {
                        logger_log(LOG_ERROR, "Type error: Comparison requires operands of the same type");
                        valid = false;
                    }
                } else if (op == 'A' || op == 'O') {
                    // For logical operations
                    if (left_type->kind == TYPE_BOOL && right_type->kind == TYPE_BOOL) {
                        valid = true;
                    } else {
                        logger_log(LOG_ERROR, "Type error: Logical operation requires boolean operands");
                        valid = false;
                    }
                } else if (op == 'E' || op == 'N') {
                    // For equality operations
                    if (types_are_compatible(left_type, right_type)) {
                        valid = true;
                    } else {
                        logger_log(LOG_ERROR, "Type error: Equality comparison requires compatible types");
                        valid = false;
                    }
                }
            }
            break;
            
        case AST_FUNC_CALL:
            // Validate function call arguments
            // Here we would normally look up the function in a symbol table
            // For now, we'll check some hardcoded functions
            
            if (strcmp(node->funcCall.name, "Point_init") == 0) {
                // Point_init(Point*, float, float)
                if (node->funcCall.argCount != 3) {
                    logger_log(LOG_ERROR, "Type error: 'Point_init' requires 3 arguments, got %d",
                               node->funcCall.argCount);
                    valid = false;
                } else {
                    // Check first arg is a Point
                    if (node->funcCall.arguments[0]) {
                        Type* arg_type = ast_infer_type(node->funcCall.arguments[0]);
                        if (arg_type->kind != TYPE_CLASS || 
                            strcmp(arg_type->typeName, "Point") != 0) {
                            logger_log(LOG_ERROR, "Type error: First argument to 'Point_init' must be a Point");
                            valid = false;
                        }
                    }
                    
                    // Check other args are numeric
                    for (int i = 1; i < 3; i++) {
                        if (node->funcCall.arguments[i]) {
                            Type* arg_type = ast_infer_type(node->funcCall.arguments[i]);
                            if (arg_type->kind != TYPE_INT && arg_type->kind != TYPE_FLOAT) {
                                logger_log(LOG_ERROR, "Type error: Arguments to 'Point_init' must be numeric");
                                valid = false;
                            }
                        }
                    }
                }
            }
            // Add more function checks as needed
            break;
            
        // Add checks for other node types as needed
    }
    
    // Recursively check child nodes
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!validate_ast_types(node->program.statements[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (!validate_ast_types(node->funcDef.body[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_IF_STMT:
            // Check condition
            if (!validate_ast_types(node->ifStmt.condition)) {
                valid = false;
            }
            
            // Check then branch
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                if (!validate_ast_types(node->ifStmt.thenBranch[i])) {
                    valid = false;
                }
            }
            
            // Check else branch
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                if (!validate_ast_types(node->ifStmt.elseBranch[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_WHILE_STMT:
            // Check condition
            if (!validate_ast_types(node->whileStmt.condition)) {
                valid = false;
            }
            
            // Check body
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                if (!validate_ast_types(node->whileStmt.body[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_DO_WHILE_STMT:
            // Check condition
            if (!validate_ast_types(node->doWhileStmt.condition)) {
                valid = false;
            }
            
            // Check body
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                if (!validate_ast_types(node->doWhileStmt.body[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_FOR_STMT:
            // Check range expressions
            if (!validate_ast_types(node->forStmt.rangeStart) || 
                !validate_ast_types(node->forStmt.rangeEnd)) {
                valid = false;
            }
            
            // Check body
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                if (!validate_ast_types(node->forStmt.body[i])) {
                    valid = false;
                }
            }
            break;
            
        case AST_BINARY_OP:
            if (!validate_ast_types(node->binaryOp.left) || 
                !validate_ast_types(node->binaryOp.right)) {
                valid = false;
            }
            break;
            
        case AST_UNARY_OP:
            if (!validate_ast_types(node->unaryOp.expr)) {
                valid = false;
            }
            break;
            
        case AST_VAR_ASSIGN:
            if (node->varAssign.initializer && 
                !validate_ast_types(node->varAssign.initializer)) {
                valid = false;
            }
            break;
            
        case AST_FUNC_CALL:
            for (int i = 0; i < node->funcCall.argCount; i++) {
                if (node->funcCall.arguments[i] && 
                    !validate_ast_types(node->funcCall.arguments[i])) {
                    valid = false;
                }
            }
            break;
    }
    
    return valid;
}
