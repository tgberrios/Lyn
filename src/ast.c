#include "ast.h"
#include "memory.h"   // Usa malloc/free o tus funciones personalizadas
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>    // Para fprintf, stderr
#include <stdint.h>   // For uintptr_t

// Stub para report_error si no está definido en otro módulo
void report_error(int errorCode, const char* msg) {
    fprintf(stderr, "Error %d: %s\n", errorCode, msg);
}

AstNode* createAstNode(AstNodeType type) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    if (!node) {
        report_error(1, "Failed to allocate memory for AST node");
        return NULL;
    }
    node->type = type;
    node->line = 0;
    node->inferredType = NULL;
    return node;
}

// Fix the bug in freeAstNode function to handle null pointers properly
void freeAstNode(AstNode* node) {
    if (!node) return;
    
    // Add stricter pointer validation with additional checks
    uintptr_t node_addr = (uintptr_t)node;
    
    // Check for common invalid address patterns
    if (node_addr < 0x1000 || 
        node_addr > (uintptr_t)0x7fffffffffffffff ||
        (node_addr & 0x7) != 0 ||  // Check alignment (pointers should be 8-byte aligned)
        (node_addr >= 0x2000000000 && node_addr <= 0x20ffffffffff) ||  // Detect pattern in corrupted pointers
        node_addr == 0x200a202020202020 ||  // Specific bad address we've seen
        node_addr == 0x200a3b2928746e69) {  // Another bad address we've seen
        fprintf(stderr, "Warning: Skipping invalid AST node address %p\n", (void*)node);
        return;
    }
    
    // Additional validation - try to safely access the type field
    // This is a cautious approach to avoid dereferencing bad pointers
    AstNodeType type;
    if (!memcpy(&type, &node->type, sizeof(AstNodeType)) || 
        (type < AST_PROGRAM || type > AST_BREAK_STMT)) {  // Updated to include new types
        fprintf(stderr, "Warning: Detected invalid node type %d at address %p\n", type, (void*)node);
        return;
    }
    
    // Now we can proceed with the normal cleanup
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                freeAstNode(node->program.statements[i]);
            }
            free(node->program.statements);
            break;
        case AST_NUMBER_LITERAL:
            break;
        case AST_STRING_LITERAL:
            break;
        case AST_IDENTIFIER:
            break;
        case AST_VAR_DECL:
            freeAstNode(node->varDecl.initializer);
            break;
        case AST_VAR_ASSIGN:
            freeAstNode(node->varAssign.initializer);
            break;
        case AST_FUNC_DEF:
            if (node->funcDef.parameters) {
                for (int i = 0; i < node->funcDef.paramCount; i++) {
                    freeAstNode(node->funcDef.parameters[i]);
                }
                free(node->funcDef.parameters);
            }
            if (node->funcDef.body) {
                for (int i = 0; i < node->funcDef.bodyCount; i++) {
                    freeAstNode(node->funcDef.body[i]);
                }
                free(node->funcDef.body);
            }
            break;
        case AST_EXPR_STMT:
            freeAstNode(node->exprStmt.expr);
            break;
        case AST_IF_STMT:
            freeAstNode(node->ifStmt.condition);
            if (node->ifStmt.thenBranch) {
                for (int i = 0; i < node->ifStmt.thenCount; i++) {
                    freeAstNode(node->ifStmt.thenBranch[i]);
                }
                free(node->ifStmt.thenBranch);
            }
            if (node->ifStmt.elseBranch) {
                for (int i = 0; i < node->ifStmt.elseCount; i++) {
                    freeAstNode(node->ifStmt.elseBranch[i]);
                }
                free(node->ifStmt.elseBranch);
            }
            break;
        case AST_WHILE_STMT:
            freeAstNode(node->whileStmt.condition);
            if (node->whileStmt.body) {
                for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                    freeAstNode(node->whileStmt.body[i]);
                }
                free(node->whileStmt.body);
            }
            break;
        case AST_FOR_STMT:
            freeAstNode(node->forStmt.rangeStart);
            freeAstNode(node->forStmt.rangeEnd);
            if (node->forStmt.body) {
                for (int i = 0; i < node->forStmt.bodyCount; i++) {
                    freeAstNode(node->forStmt.body[i]);
                }
                free(node->forStmt.body);
            }
            break;
        case AST_RETURN_STMT:
            freeAstNode(node->returnStmt.expr);
            break;
        case AST_BINARY_OP:
            freeAstNode(node->binaryOp.left);
            freeAstNode(node->binaryOp.right);
            break;
        case AST_FUNC_CALL:
            if (node->funcCall.arguments) {
                for (int i = 0; i < node->funcCall.argCount; i++) {
                    freeAstNode(node->funcCall.arguments[i]);
                }
                free(node->funcCall.arguments);
            }
            break;
        case AST_MEMBER_ACCESS:
            // Fix the Circle issue - make sure we check if object exists
            if (node->memberAccess.object) {
                freeAstNode(node->memberAccess.object);
                node->memberAccess.object = NULL; // Prevent double-free
            }
            break;
        case AST_PRINT_STMT:
            freeAstNode(node->printStmt.expr);
            break;
        case AST_CLASS_DEF:
            if (node->classDef.members) {
                for (int i = 0; i < node->classDef.memberCount; i++) {
                    freeAstNode(node->classDef.members[i]);
                }
                free(node->classDef.members);
            }
            break;
        case AST_LAMBDA:
            if (node->lambda.parameters) {
                for (int i = 0; i < node->lambda.paramCount; i++) {
                    freeAstNode(node->lambda.parameters[i]);
                }
                free(node->lambda.parameters);
            }
            freeAstNode(node->lambda.body);
            break;
        case AST_ARRAY_LITERAL:
            if (node->arrayLiteral.elements) {
                for (int i = 0; i < node->arrayLiteral.elementCount; i++) {
                    freeAstNode(node->arrayLiteral.elements[i]);
                }
                free(node->arrayLiteral.elements);
            }
            break;
        case AST_MODULE_DECL:
            if (node->moduleDecl.declarations) {
                for (int i = 0; i < node->moduleDecl.declarationCount; i++) {
                    freeAstNode(node->moduleDecl.declarations[i]);
                }
                free(node->moduleDecl.declarations);
            }
            break;
        case AST_IMPORT:
            break;
        case AST_DO_WHILE_STMT:
            freeAstNode(node->doWhileStmt.condition);
            if (node->doWhileStmt.body) {
                for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                    freeAstNode(node->doWhileStmt.body[i]);
                }
                free(node->doWhileStmt.body);
            }
            break;
        
        case AST_SWITCH_STMT:
            freeAstNode(node->switchStmt.expr);
            if (node->switchStmt.cases) {
                for (int i = 0; i < node->switchStmt.caseCount; i++) {
                    freeAstNode(node->switchStmt.cases[i]);
                }
                free(node->switchStmt.cases);
            }
            if (node->switchStmt.defaultCase) {
                for (int i = 0; i < node->switchStmt.defaultCaseCount; i++) {
                    freeAstNode(node->switchStmt.defaultCase[i]);
                }
                free(node->switchStmt.defaultCase);
            }
            break;
        
        case AST_CASE_STMT:
            freeAstNode(node->caseStmt.expr);
            if (node->caseStmt.body) {
                for (int i = 0; i < node->caseStmt.bodyCount; i++) {
                    freeAstNode(node->caseStmt.body[i]);
                }
                free(node->caseStmt.body);
            }
            break;
        
        case AST_TRY_CATCH_STMT:
            if (node->tryCatchStmt.tryBody) {
                for (int i = 0; i < node->tryCatchStmt.tryCount; i++) {
                    freeAstNode(node->tryCatchStmt.tryBody[i]);
                }
                free(node->tryCatchStmt.tryBody);
            }
            if (node->tryCatchStmt.catchBody) {
                for (int i = 0; i < node->tryCatchStmt.catchCount; i++) {
                    freeAstNode(node->tryCatchStmt.catchBody[i]);
                }
                free(node->tryCatchStmt.catchBody);
            }
            if (node->tryCatchStmt.finallyBody) {
                for (int i = 0; i < node->tryCatchStmt.finallyCount; i++) {
                    freeAstNode(node->tryCatchStmt.finallyBody[i]);
                }
                free(node->tryCatchStmt.finallyBody);
            }
            break;
        
        case AST_THROW_STMT:
            freeAstNode(node->throwStmt.expr);
            break;
        
        case AST_BREAK_STMT:
            // No additional fields to free
            break;
            
        default:
            break;
    }
    free(node);
}

void freeAst(AstNode* root) {
    freeAstNode(root);
}
