#include "ast.h"
#include "types.h"
#include "logger.h"
#include "error.h"
#include <stdio.h>

// Asignar un tipo a un nodo AST
void ast_set_type(AstNode* node, Type* type) {
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to set type on NULL AST node");
        return;
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_set_type);
    
    // En este contexto, simplemente guardamos el tipo en una estructura auxiliar
    // Esto es una simplificación, en un compilador real habría que hacer más
    node->inferredType = type;
    
    logger_log(LOG_DEBUG, "Set AST node type: %s", typeToString(type));
}

// Inferir el tipo de una expresión
Type* ast_infer_type(AstNode* node) {
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to infer type of NULL AST node");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    error_push_debug(__func__, __FILE__, __LINE__, (void*)ast_infer_type);
    
    Type* result = NULL;
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            result = create_primitive_type(TYPE_FLOAT);
            break;
            
        case AST_STRING_LITERAL:
            result = create_primitive_type(TYPE_STRING);
            break;
            
        case AST_BINARY_OP:
            // Simplificación: asumimos que todas las operaciones binarias 
            // con números devuelven números
            // En un compilador real, habría que verificar los tipos de los operandos
            result = create_primitive_type(TYPE_FLOAT);
            break;
            
        case AST_FUNC_CALL:
            // Simplificación: asumimos que todas las funciones devuelven void
            // En un compilador real, habría que buscar la declaración de la función
            result = create_primitive_type(TYPE_VOID);
            break;
            
        // Agregar más casos según sea necesario
            
        default:
            result = create_primitive_type(TYPE_UNKNOWN);
            logger_log(LOG_DEBUG, "Unknown type for AST node type %d", node->type);
            break;
    }
    
    logger_log(LOG_DEBUG, "Inferred type for AST node type %d: %s", 
              node->type, typeToString(result));
    
    return result;
}
