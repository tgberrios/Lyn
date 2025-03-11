/* ast.c */
#include "ast.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* La función initAstPool y la variable astPoolMemory no se usan en esta versión.
   Se pueden eliminar o dejar comentadas para evitar advertencias. */
#if 0
static void initAstPool(void) {
    /* Inicialización del pool de memoria para el AST */
}

static void *astPoolMemory = NULL;  // No se usa directamente, se maneja a través de memory_pool_create.
#endif

/* Crea un nuevo nodo AST del tipo especificado */
AstNode *createAstNode(AstNodeType type) {
    AstNode *node = memory_alloc(sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nodo AST\n");
        exit(1);
    }
    node->type = type;
    switch (type) {
        case AST_PROGRAM:
            node->program.statements = NULL;
            node->program.statementCount = 0;
            break;
        case AST_VAR_ASSIGN:
            memset(node->varAssign.name, 0, sizeof(node->varAssign.name));
            node->varAssign.initializer = NULL;
            break;
        case AST_VAR_DECL:
            memset(node->varDecl.name, 0, sizeof(node->varDecl.name));
            memset(node->varDecl.type, 0, sizeof(node->varDecl.type));
            node->varDecl.initializer = NULL;
            break;
        case AST_FUNC_DEF:
            memset(node->funcDef.name, 0, sizeof(node->funcDef.name));
            node->funcDef.parameters = NULL;
            node->funcDef.paramCount = 0;
            memset(node->funcDef.returnType, 0, sizeof(node->funcDef.returnType));
            node->funcDef.body = NULL;
            node->funcDef.bodyCount = 0;
            break;
        case AST_FUNC_CALL:
            memset(node->funcCall.name, 0, sizeof(node->funcCall.name));
            node->funcCall.arguments = NULL;
            node->funcCall.argCount = 0;
            break;
        case AST_RETURN_STMT:
            node->returnStmt.expr = NULL;
            break;
        case AST_PRINT_STMT:
            node->printStmt.expr = NULL;
            break;
        case AST_LAMBDA:
            node->lambda.parameters = NULL;
            node->lambda.paramCount = 0;
            memset(node->lambda.returnType, 0, sizeof(node->lambda.returnType));
            node->lambda.body = NULL;
            break;
        case AST_CLASS_DEF:
            memset(node->classDef.name, 0, sizeof(node->classDef.name));
            node->classDef.members = NULL;
            node->classDef.memberCount = 0;
            node->classDef.baseClassName[0] = '\0';
            break;
        case AST_IF_STMT:
            node->ifStmt.condition = NULL;
            node->ifStmt.thenBranch = NULL;
            node->ifStmt.thenCount = 0;
            node->ifStmt.elseBranch = NULL;
            node->ifStmt.elseCount = 0;
            break;
        case AST_FOR_STMT:
            memset(node->forStmt.iterator, 0, sizeof(node->forStmt.iterator));
            node->forStmt.rangeStart = NULL;
            node->forStmt.rangeEnd = NULL;
            node->forStmt.body = NULL;
            node->forStmt.bodyCount = 0;
            break;
        case AST_IMPORT:
            memset(node->importStmt.moduleType, 0, sizeof(node->importStmt.moduleType));
            memset(node->importStmt.moduleName, 0, sizeof(node->importStmt.moduleName));
            break;
        case AST_ARRAY_LITERAL:
            node->arrayLiteral.elements = NULL;
            node->arrayLiteral.elementCount = 0;
            break;
        case AST_BINARY_OP:
            node->binaryOp.left = NULL;
            node->binaryOp.op = '\0';
            node->binaryOp.right = NULL;
            break;
        case AST_NUMBER_LITERAL:
            node->numberLiteral.value = 0;
            break;
        case AST_STRING_LITERAL:
            memset(node->stringLiteral.value, 0, sizeof(node->stringLiteral.value));
            break;
        case AST_IDENTIFIER:
            memset(node->identifier.name, 0, sizeof(node->identifier.name));
            break;
        case AST_MEMBER_ACCESS:
            node->memberAccess.object = NULL;
            memset(node->memberAccess.member, 0, sizeof(node->memberAccess.member));
            break;
        case AST_METHOD_CALL:
            node->methodCall.object = NULL;
            memset(node->methodCall.method, 0, sizeof(node->methodCall.method));
            node->methodCall.arguments = NULL;
            node->methodCall.argCount = 0;
            break;
        default:
            break;
    }
    return node;
}

/* Libera un nodo AST y todos sus descendientes */
void freeAstNode(AstNode *node) {
    if (!node)
        return;
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                freeAstNode(node->program.statements[i]);
            }
            if (node->program.statements)
                memory_free(node->program.statements);
            break;
        case AST_VAR_ASSIGN:
            freeAstNode(node->varAssign.initializer);
            break;
        case AST_VAR_DECL:
            freeAstNode(node->varDecl.initializer);
            break;
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.paramCount; i++) {
                freeAstNode(node->funcDef.parameters[i]);
            }
            if (node->funcDef.parameters)
                memory_free(node->funcDef.parameters);
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                freeAstNode(node->funcDef.body[i]);
            }
            if (node->funcDef.body)
                memory_free(node->funcDef.body);
            break;
        case AST_FUNC_CALL:
            for (int i = 0; i < node->funcCall.argCount; i++) {
                freeAstNode(node->funcCall.arguments[i]);
            }
            if (node->funcCall.arguments)
                memory_free(node->funcCall.arguments);
            break;
        case AST_RETURN_STMT:
            freeAstNode(node->returnStmt.expr);
            break;
        case AST_PRINT_STMT:
            freeAstNode(node->printStmt.expr);
            break;
        case AST_LAMBDA:
            for (int i = 0; i < node->lambda.paramCount; i++) {
                freeAstNode(node->lambda.parameters[i]);
            }
            if (node->lambda.parameters)
                memory_free(node->lambda.parameters);
            freeAstNode(node->lambda.body);
            break;
        case AST_CLASS_DEF:
            for (int i = 0; i < node->classDef.memberCount; i++) {
                freeAstNode(node->classDef.members[i]);
            }
            if (node->classDef.members)
                memory_free(node->classDef.members);
            break;
        case AST_IF_STMT:
            freeAstNode(node->ifStmt.condition);
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                freeAstNode(node->ifStmt.thenBranch[i]);
            }
            if (node->ifStmt.thenBranch)
                memory_free(node->ifStmt.thenBranch);
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                freeAstNode(node->ifStmt.elseBranch[i]);
            }
            if (node->ifStmt.elseBranch)
                memory_free(node->ifStmt.elseBranch);
            break;
        case AST_FOR_STMT:
            freeAstNode(node->forStmt.rangeStart);
            freeAstNode(node->forStmt.rangeEnd);
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                freeAstNode(node->forStmt.body[i]);
            }
            if (node->forStmt.body)
                memory_free(node->forStmt.body);
            break;
        case AST_IMPORT:
            break;
        case AST_ARRAY_LITERAL:
            for (int i = 0; i < node->arrayLiteral.elementCount; i++) {
                freeAstNode(node->arrayLiteral.elements[i]);
            }
            if (node->arrayLiteral.elements)
                memory_free(node->arrayLiteral.elements);
            break;
        case AST_BINARY_OP:
            freeAstNode(node->binaryOp.left);
            freeAstNode(node->binaryOp.right);
            break;
        case AST_NUMBER_LITERAL:
        case AST_STRING_LITERAL:
        case AST_IDENTIFIER:
        case AST_MEMBER_ACCESS:
        case AST_METHOD_CALL:
            /* Estos nodos no tienen descendientes propios */
            break;
        default:
            break;
    }
    memory_free(node);
}
