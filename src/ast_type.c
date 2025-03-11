#include "ast.h"
#include "types.h"
#include <stdio.h>

// Asignar un tipo a un nodo AST
void ast_set_type(AstNode* node, Type* type) {
    if (!node) return;
    
    // En este contexto, simplemente guardamos el tipo en una estructura auxiliar
    // Esto es una simplificación, en un compilador real habría que hacer más
    
    // Este código es solo un esqueleto y probablemente necesite ser modificado
    // según la implementación real de los tipos en el compilador
    // node->type = type;
    printf("Setting type for AST node: %s\n", typeToString(type));
}

// Inferir el tipo de una expresión
Type* ast_infer_type(AstNode* node) {
    if (!node) return create_primitive_type(TYPE_UNKNOWN);
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            return create_primitive_type(TYPE_FLOAT);
            
        case AST_STRING_LITERAL:
            return create_primitive_type(TYPE_STRING);
            
        case AST_BINARY_OP:
            // Simplificación: asumimos que todas las operaciones binarias 
            // con números devuelven números
            // En un compilador real, habría que verificar los tipos de los operandos
            return create_primitive_type(TYPE_FLOAT);
            
        case AST_FUNC_CALL:
            // Simplificación: asumimos que todas las funciones devuelven void
            // En un compilador real, habría que buscar la declaración de la función
            return create_primitive_type(TYPE_VOID);
            
        // Agregar más casos según sea necesario
            
        default:
            return create_primitive_type(TYPE_UNKNOWN);
    }
}
