#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ast.h"
#include "types.h"

// Template parameter info
typedef struct {
    char name[256];
    Type* constraint;  // Type constraint (can be NULL)
} TemplateParam;

// Template definition
typedef struct {
    char name[256];
    TemplateParam** params;
    int paramCount;
    AstNode* body;
} TemplateDefinition;

// Template instantiation
typedef struct {
    const char* templateName;
    Type** typeArgs;
    int typeArgCount;
} TemplateInstance;

// Template API
bool register_template(const char* name, TemplateParam** params, int paramCount, AstNode* body);
AstNode* instantiate_template(const char* name, Type** typeArgs, int typeArgCount);
void optimize_template(AstNode* node);
bool validate_template_constraints(TemplateInstance* instance);

// AST manipulation utilities
AstNode* clone_ast_node(AstNode* node);
AstNode* substitute_type_params(AstNode* node, const char** paramNames, Type** typeArgs, int count);
AstNode* inline_template_calls(AstNode* node);
void specialize_generic_code(AstNode* node, Type** typeArgs, int typeArgCount);

#endif
