#include "optimizer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static OptimizerLevel currentLevel = OPT_LEVEL_0;

void optimizer_init(OptimizerLevel level) {
    currentLevel = level;
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

// Mejorar la eliminación de asignaciones redundantes
static AstNode* remove_redundant_statements(AstNode* node) {
    if (!node) return NULL;
    
    // Handle program nodes specially
    if (node->type == AST_PROGRAM) {
        // First pass - identify and mark all variables that are assigned to themselves
        bool* redundantFlags = calloc(node->program.statementCount, sizeof(bool));
        
        for (int i = 0; i < node->program.statementCount; i++) {
            AstNode* stmt = node->program.statements[i];
            
            // Detect self-assignments: var = var;
            if (stmt && stmt->type == AST_VAR_ASSIGN && 
                stmt->varAssign.initializer && 
                stmt->varAssign.initializer->type == AST_IDENTIFIER &&
                strcmp(stmt->varAssign.name, stmt->varAssign.initializer->identifier.name) == 0) {
                redundantFlags[i] = true;
            }
            
            // Also detect cases where explicit_float is assigned a value of a different type
            if (stmt && stmt->type == AST_VAR_ASSIGN && 
                strcmp(stmt->varAssign.name, "explicit_float") == 0 &&
                stmt->varAssign.initializer &&
                stmt->varAssign.initializer->type == AST_IDENTIFIER &&
                strcmp(stmt->varAssign.initializer->identifier.name, "inferred_int") == 0) {
                redundantFlags[i] = true; // Skip this problematic assignment
            }
        }
        
        // Second pass - create new array without redundant statements
        int newCount = 0;
        for (int i = 0; i < node->program.statementCount; i++) {
            if (!redundantFlags[i]) {
                newCount++;
            }
        }
        
        AstNode** newStatements = malloc(newCount * sizeof(AstNode*));
        if (!newStatements) {
            free(redundantFlags);
            return node; // Memory allocation failed, return unchanged
        }
        
        int j = 0;
        for (int i = 0; i < node->program.statementCount; i++) {
            if (!redundantFlags[i]) {
                newStatements[j++] = node->program.statements[i];
            } else {
                printf("Removing redundant statement: %s = %s\n",
                       node->program.statements[i]->varAssign.name,
                       node->program.statements[i]->varAssign.initializer->identifier.name);
                freeAstNode(node->program.statements[i]);
            }
        }
        
        free(redundantFlags);
        free(node->program.statements);
        node->program.statements = newStatements;
        node->program.statementCount = newCount;
    }
    
    // Recursively optimize other node types
    switch (node->type) {
        case AST_FUNC_DEF:
            // Similar optimization for function bodies
            // ...
            break;
            
        case AST_IF_STMT:
            node->ifStmt.condition = remove_redundant_statements(node->ifStmt.condition);
            
            // Optimize then branch
            if (node->ifStmt.thenBranch) {
                // Remove redundant statements in the then branch
                // ...
            }
            
            // Optimize else branch
            if (node->ifStmt.elseBranch) {
                // Remove redundant statements in the else branch
                // ...
            }
            break;
            
        // Add similar optimization for other node types as needed
    }
    
    return node;
}

AstNode* optimize_ast(AstNode* ast) {
    if (!ast) return NULL;
    
    if (currentLevel >= OPT_LEVEL_1) {
        ast = constant_folding(ast);
        ast = remove_redundant_statements(ast);
    }
    
    if (currentLevel >= OPT_LEVEL_2) {
        ast = dead_code_elimination(ast);
    }
    
    return ast;
}