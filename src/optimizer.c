#include "optimizer.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static OptimizationLevel current_level = OPT_LEVEL_0;

void optimizer_init(OptimizationLevel level) {
    current_level = level;
    logger_log(LOG_INFO, "Inicializando optimizador en nivel %d", level);
}

static AstNode* constant_folding(AstNode* node) {
    if (!node) return NULL;

    // Primero optimizar recursivamente los nodos hijos
    switch (node->type) {
        case AST_BINARY_OP:
            node->binaryOp.left = constant_folding(node->binaryOp.left);
            node->binaryOp.right = constant_folding(node->binaryOp.right);
            
            // Si ambos operandos son constantes, evaluar en tiempo de compilación
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
                    case '/': 
                        if (right == 0) {
                            logger_log(LOG_WARNING, "División por cero detectada en optimización");
                            return node;
                        }
                        result = left / right; 
                        break;
                    default: return node;
                }

                logger_log(LOG_DEBUG, "Constant folding: %f %c %f = %f",
                          left, node->binaryOp.op, right, result);

                // Crear un nuevo nodo literal con el resultado
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

        default:
            break;
    }

    return node;
}

static AstNode* dead_code_elimination(AstNode* node) {
    if (!node) return NULL;

    switch (node->type) {
        case AST_FUNC_DEF: {
            int newBodyCount = 0;
            bool foundReturn = false;

            // Eliminar código después de return
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (foundReturn) {
                    logger_log(LOG_DEBUG, "Eliminando código muerto después de return");
                    freeAstNode(node->funcDef.body[i]);
                    continue;
                }
                if (node->funcDef.body[i]->type == AST_RETURN_STMT) {
                    foundReturn = true;
                }
                node->funcDef.body[newBodyCount++] = node->funcDef.body[i];
            }
            node->funcDef.bodyCount = newBodyCount;
            break;
        }

        case AST_IF_STMT:
            // Optimizar condición if
            node->ifStmt.condition = dead_code_elimination(node->ifStmt.condition);
            
            // Si la condición es una constante, podemos eliminar una rama
            if (node->ifStmt.condition->type == AST_NUMBER_LITERAL) {
                bool condition = node->ifStmt.condition->numberLiteral.value != 0;
                
                if (condition) {
                    // La condición es verdadera, eliminar rama else
                    logger_log(LOG_DEBUG, "Eliminando rama else (condición siempre verdadera)");
                    for (int i = 0; i < node->ifStmt.elseCount; i++) {
                        freeAstNode(node->ifStmt.elseBranch[i]);
                    }
                    node->ifStmt.elseCount = 0;
                } else {
                    // La condición es falsa, eliminar rama then
                    logger_log(LOG_DEBUG, "Eliminando rama then (condición siempre falsa)");
                    for (int i = 0; i < node->ifStmt.thenCount; i++) {
                        freeAstNode(node->ifStmt.thenBranch[i]);
                    }
                    node->ifStmt.thenCount = 0;
                }
            }
            break;

        default:
            break;
    }

    return node;
}

AstNode* optimize_ast(AstNode* ast) {
    if (!ast || current_level == OPT_LEVEL_0) return ast;

    logger_log(LOG_INFO, "Iniciando optimización de AST (nivel %d)", current_level);

    // Aplicar optimizaciones según el nivel
    if (current_level >= OPT_LEVEL_1) {
        ast = constant_folding(ast);
        logger_log(LOG_DEBUG, "Constant folding completado");
    }

    if (current_level >= OPT_LEVEL_2) {
        ast = dead_code_elimination(ast);
        logger_log(LOG_DEBUG, "Eliminación de código muerto completada");
    }

    logger_log(LOG_INFO, "Optimización de AST completada");
    return ast;
}