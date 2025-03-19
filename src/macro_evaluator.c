#include <stdio.h>
#include <stdlib.h>
#include "macro_evaluator.h"
#include "error.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_MACROS 1024

// Structure to store macro definitions
typedef struct {
    char name[256];
    char** params;
    int paramCount;
    AstNode** body;
    int bodyCount;
} MacroDef;

static MacroDef macros[MAX_MACROS];
static int macroCount = 0;

bool register_macro(AstNode* node) {
    if (node->type != AST_MACRO_DEF || macroCount >= MAX_MACROS) {
        return false;
    }

    MacroDef* macro = &macros[macroCount++];
    strncpy(macro->name, node->macroDef.name, sizeof(macro->name) - 1);
    
    // Copy parameters
    macro->paramCount = node->macroDef.paramCount;
    macro->params = malloc(macro->paramCount * sizeof(char*));
    for (int i = 0; i < macro->paramCount; i++) {
        macro->params[i] = strdup(node->macroDef.params[i]);
    }
    
    // Copy body
    macro->bodyCount = node->macroDef.bodyCount;
    macro->body = malloc(macro->bodyCount * sizeof(AstNode*));
    for (int i = 0; i < macro->bodyCount; i++) {
        macro->body[i] = node->macroDef.body[i];
    }
    
    return true;
}

static MacroDef* find_macro(const char* name) {
    for (int i = 0; i < macroCount; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return &macros[i];
        }
    }
    return NULL;
}

AstNode* expand_macro(const char* name, AstNode** args, int argCount) {
    MacroDef* macro = find_macro(name);
    if (!macro || macro->paramCount != argCount) {
        return NULL;
    }
    
    // Create new scope for macro expansion
    // Map parameters to arguments
    AstNode** expanded = malloc(macro->bodyCount * sizeof(AstNode*));
    
    // Replace parameter references with actual arguments
    for (int i = 0; i < macro->bodyCount; i++) {
        AstNode* stmt = macro->body[i];
        if (stmt->type == AST_MACRO_PARAM) {
            int paramIndex = stmt->macroParam.index;
            if (paramIndex < argCount) {
                expanded[i] = args[paramIndex];
            }
        } else {
            expanded[i] = stmt;
        }
    }
    
    // Create block node with expanded macro body
    AstNode* block = createAstNode(AST_PROGRAM);
    block->program.statements = expanded;
    block->program.statementCount = macro->bodyCount;
    
    return block;
}

char* macro_stringify(AstNode* node) {
    char buffer[1024];
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            snprintf(buffer, sizeof(buffer), "%g", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            snprintf(buffer, sizeof(buffer), "\"%s\"", node->stringLiteral.value);
            break;
        case AST_IDENTIFIER:
            snprintf(buffer, sizeof(buffer), "%s", node->identifier.name);
            break;
        default:
            return strdup("<<unprintable>>");
    }
    
    return strdup(buffer);
}

char* macro_concat(const char* s1, const char* s2) {
    size_t len = strlen(s1) + strlen(s2) + 1;
    char* result = malloc(len);
    if (result) {
        snprintf(result, len, "%s%s", s1, s2);
    }
    return result;
}

AstNode* evaluate_macros(AstNode* node) {
    if (!node) return NULL;

    // Register macro definitions
    if (node->type == AST_MACRO_DEF) {
        register_macro(node);
        return NULL; // Remove macro definition from AST
    }
    
    // Expand macro invocations
    if (node->type == AST_MACRO_EXPAND) {
        return expand_macro(node->macroExpand.name,
                          node->macroExpand.args,
                          node->macroExpand.argCount);
    }
    
    // Recursively process children
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                node->program.statements[i] = evaluate_macros(node->program.statements[i]);
            }
            break;
            
        // Add cases for other node types that can contain child nodes
        // ...
    }
    
    return node;
}
