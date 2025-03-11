#include "optimizer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int optimization_level = OPT_LEVEL_0;

void optimizer_init(int level) {
    optimization_level = level;
}

/**
 * Realiza constante folding (evaluación de expresiones constantes)
 */
static AstNode* constant_folding(AstNode* node) {
    if (!node) return NULL;

    switch (node->type) {
        case AST_BINARY_OP:
            node->binaryOp.left = constant_folding(node->binaryOp.left);
            node->binaryOp.right = constant_folding(node->binaryOp.right);
            
            // Si ambos operandos son constantes, evaluar la expresión
            if (node->binaryOp.left && node->binaryOp.right &&
                node->binaryOp.left->type == AST_NUMBER_LITERAL &&
                node->binaryOp.right->type == AST_NUMBER_LITERAL) {
                
                double left = node->binaryOp.left->numberLiteral.value;
                double right = node->binaryOp.right->numberLiteral.value;
                double result = 0;
                
                switch (node->binaryOp.op) {
                    case '+': result = left + right; break;
                    case '-': result = left - right; break;
                    case '*': result = left * right; break;
                    case '/': result = left / right; break;
                }
                
                printf("Constant folding: %g %c %g = %g\n",
                           left, node->binaryOp.op, right, result);
                
                AstNode* optimized = createAstNode(AST_NUMBER_LITERAL);
                optimized->numberLiteral.value = result;
                
                // Liberar el nodo original
                freeAstNode(node);
                return optimized;
            }
            break;
            
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                node->funcDef.body[i] = constant_folding(node->funcDef.body[i]);
            }
            break;
            
        case AST_IF_STMT:
            node->ifStmt.condition = constant_folding(node->ifStmt.condition);
            
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                node->ifStmt.thenBranch[i] = constant_folding(node->ifStmt.thenBranch[i]);
            }
            
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                node->ifStmt.elseBranch[i] = constant_folding(node->ifStmt.elseBranch[i]);
            }
            break;
    }
    
    return node;
}

/**
 * Elimina código muerto (código que nunca se ejecutará)
 */
static AstNode* dead_code_elimination(AstNode* node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_FUNC_DEF: {
            // Verificar si hay un return que corte la ejecución antes del final
            int hasEarlyReturn = 0;
            int newBodyCount = 0;
            
            // Primero procesamos recursivamente cada instrucción
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                // Si ya encontramos un return, todo lo que sigue es código muerto
                if (hasEarlyReturn) {
                    printf("Eliminating dead code after return in function %s\n", node->funcDef.name);
                    freeAstNode(node->funcDef.body[i]);
                    continue;
                }
                
                // Si es un return, marcamos que encontramos un return temprano
                if (node->funcDef.body[i]->type == AST_RETURN_STMT) {
                    hasEarlyReturn = 1;
                }
                
                node->funcDef.body[newBodyCount++] = node->funcDef.body[i];
            }
            
            node->funcDef.bodyCount = newBodyCount;
            break;
        }
            
        case AST_IF_STMT:
            // Optimizar la condición
            node->ifStmt.condition = constant_folding(node->ifStmt.condition);
            
            // Si la condición es una constante, podemos eliminar ramas muertas
            if (node->ifStmt.condition->type == AST_NUMBER_LITERAL) {
                bool condition = node->ifStmt.condition->numberLiteral.value != 0;
                
                if (condition) {
                    // La rama 'true' siempre se ejecutará, podemos eliminar la rama 'else'
                    for (int i = 0; i < node->ifStmt.elseCount; i++) {
                        freeAstNode(node->ifStmt.elseBranch[i]);
                    }
                    
                    node->ifStmt.elseCount = 0;
                } else {
                    // La rama 'false' siempre se ejecutará, podemos eliminar la rama 'then'
                    for (int i = 0; i < node->ifStmt.thenCount; i++) {
                        freeAstNode(node->ifStmt.thenBranch[i]);
                    }
                    
                    node->ifStmt.thenCount = 0;
                }
            }
            break;
    }
    
    return node;
}

/**
 * Aplica todas las optimizaciones configuradas al AST
 */
AstNode* optimize_ast(AstNode* ast) {
    if (optimization_level == OPT_LEVEL_0) {
        return ast; // Sin optimizaciones
    }
    
    // Aplicar optimizaciones en orden
    if (optimization_level >= OPT_LEVEL_1) {
        ast = constant_folding(ast);
    }
    
    if (optimization_level >= OPT_LEVEL_2) {
        ast = dead_code_elimination(ast);
    }
    
    return ast;
}