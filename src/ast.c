#include "ast.h"
#include "memory.h"   // Usa malloc/free o tus funciones personalizadas
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>    // Para fprintf, stderr

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

void freeAstNode(AstNode* node) {
    if (!node) return;
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
            freeAstNode(node->whileStmt.body);
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
            freeAstNode(node->memberAccess.object);
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
        default:
            break;
    }
    free(node);
}

void freeAst(AstNode* root) {
    freeAstNode(root);
}
