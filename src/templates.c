/**
 * @file templates.c
 * @brief Implementation of template system for the Lyn compiler
 * 
 * This file implements the template system that enables generic programming
 * in the Lyn language. It provides:
 * - Template registration and management
 * - Template instantiation with type arguments
 * - Type constraint validation
 * - Template specialization and optimization
 * - AST node cloning and manipulation
 */

#include <stdio.h>
#include <stdlib.h>
#include "templates.h"
#include "error.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

/** Maximum number of templates that can be registered */
#define MAX_TEMPLATES 1024

/** Registry of all template definitions */
static TemplateDefinition templates[MAX_TEMPLATES];
/** Current number of registered templates */
static int templateCount = 0;

/**
 * @brief Registers a new template definition
 * 
 * Adds a new template to the registry with its parameters and body.
 * The template can later be instantiated with specific type arguments.
 * 
 * @param name Name of the template
 * @param params Array of template parameters
 * @param paramCount Number of parameters
 * @param body AST node representing the template body
 * @return bool true if registration successful, false if limit reached
 */
bool register_template(const char* name, TemplateParam** params, int paramCount, AstNode* body) {
    if (templateCount >= MAX_TEMPLATES) {
        error_report("Template", 0, 0, "Maximum number of templates exceeded", ERROR_LIMIT);
        return false;
    }

    TemplateDefinition* template = &templates[templateCount++];
    strncpy(template->name, name, sizeof(template->name) - 1);
    template->params = params;
    template->paramCount = paramCount;
    template->body = clone_ast_node(body);

    return true;
}

/**
 * @brief Instantiates a template with specific type arguments
 * 
 * Creates a new instance of a template by substituting type parameters
 * with concrete types. The process includes:
 * - Finding the template definition
 * - Validating type arguments
 * - Substituting type parameters
 * - Specializing the code
 * - Optimizing the result
 * 
 * @param name Name of the template to instantiate
 * @param typeArgs Array of type arguments
 * @param typeArgCount Number of type arguments
 * @return AstNode* Instantiated template AST, or NULL on error
 */
AstNode* instantiate_template(const char* name, Type** typeArgs, int typeArgCount) {
    // Find template definition
    TemplateDefinition* template = NULL;
    for (int i = 0; i < templateCount; i++) {
        if (strcmp(templates[i].name, name) == 0) {
            template = &templates[i];
            break;
        }
    }

    if (!template) {
        error_report("Template", 0, 0, "Template not found", ERROR_NAME);
        return NULL;
    }

    // Validate argument count
    if (typeArgCount != template->paramCount) {
        error_report("Template", 0, 0, "Wrong number of template arguments", ERROR_TYPE);
        return NULL;
    }

    // Create instance info
    TemplateInstance instance = {
        .templateName = name,
        .typeArgs = typeArgs,
        .typeArgCount = typeArgCount
    };

    // Validate type constraints
    if (!validate_template_constraints(&instance)) {
        return NULL;
    }

    // Clone template body
    AstNode* instantiated = clone_ast_node(template->body);

    // Create parameter name array for substitution
    const char** paramNames = malloc(typeArgCount * sizeof(char*));
    for (int i = 0; i < typeArgCount; i++) {
        paramNames[i] = template->params[i]->name;
    }

    // Substitute type parameters
    instantiated = substitute_type_params(instantiated, paramNames, typeArgs, typeArgCount);

    // Specialize the code for the given types
    specialize_generic_code(instantiated, typeArgs, typeArgCount);

    // Optimize the instantiated template
    optimize_template(instantiated);

    free(paramNames);
    return instantiated;
}

/**
 * @brief Validates type arguments against template constraints
 * 
 * Checks if each type argument satisfies its corresponding template parameter
 * constraint. This ensures type safety during template instantiation.
 * 
 * @param instance The template instance to validate
 * @return bool true if all constraints are satisfied, false otherwise
 */
bool validate_template_constraints(TemplateInstance* instance) {
    TemplateDefinition* template = NULL;
    for (int i = 0; i < templateCount; i++) {
        if (strcmp(templates[i].name, instance->templateName) == 0) {
            template = &templates[i];
            break;
        }
    }

    if (!template) return false;

    // Check each type argument against its constraint
    for (int i = 0; i < instance->typeArgCount; i++) {
        Type* constraint = template->params[i]->constraint;
        if (constraint) {
            if (!are_types_compatible(instance->typeArgs[i], constraint)) {
                char error[512];
                snprintf(error, sizeof(error), 
                        "Type argument %s does not satisfy constraint %s",
                        typeToString(instance->typeArgs[i]),
                        typeToString(constraint));
                error_report("Template", 0, 0, error, ERROR_TYPE);
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Creates a deep copy of an AST node
 * 
 * Recursively clones an AST node and all its children, including
 * type information and node-specific data.
 * 
 * @param node The AST node to clone
 * @return AstNode* A deep copy of the node, or NULL if input is NULL
 */
AstNode* clone_ast_node(AstNode* node) {
    if (!node) return NULL;

    AstNode* clone = createAstNode(node->type);
    clone->line = node->line;
    clone->inferredType = node->inferredType ? clone_type(node->inferredType) : NULL;

    // Clone node-specific data
    switch (node->type) {
        case AST_IDENTIFIER:
            strncpy(clone->identifier.name, node->identifier.name, 
                   sizeof(clone->identifier.name));
            break;
            
        case AST_FUNC_DEF:
            strncpy(clone->funcDef.name, node->funcDef.name, 
                   sizeof(clone->funcDef.name));
            clone->funcDef.paramCount = node->funcDef.paramCount;
            clone->funcDef.parameters = malloc(node->funcDef.paramCount * sizeof(AstNode*));
            for (int i = 0; i < node->funcDef.paramCount; i++) {
                clone->funcDef.parameters[i] = clone_ast_node(node->funcDef.parameters[i]);
            }
            // Clone body statements
            clone->funcDef.bodyCount = node->funcDef.bodyCount;
            clone->funcDef.body = malloc(node->funcDef.bodyCount * sizeof(AstNode*));
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                clone->funcDef.body[i] = clone_ast_node(node->funcDef.body[i]);
            }
            break;
            
        // Add cases for other node types as needed
    }

    return clone;
}

/**
 * @brief Substitutes type parameters in an AST with concrete types
 * 
 * Replaces template parameter names with their corresponding concrete type
 * names throughout the AST. This is part of the template instantiation process.
 * 
 * @param node The AST node to process
 * @param paramNames Array of parameter names to replace
 * @param typeArgs Array of concrete types to substitute
 * @param count Number of parameters/types
 * @return AstNode* Modified AST with type parameters substituted
 */
AstNode* substitute_type_params(AstNode* node, const char** paramNames, Type** typeArgs, int count) {
    if (!node) return NULL;

    // Handle type annotations and references
    if (node->type == AST_VAR_DECL) {
        // Check if the type name matches any template parameter
        for (int i = 0; i < count; i++) {
            if (strcmp(node->varDecl.type, paramNames[i]) == 0) {
                // Replace with actual type name
                strncpy(node->varDecl.type, typeToString(typeArgs[i]),
                       sizeof(node->varDecl.type) - 1);
                break;
            }
        }
    }

    // Recursively process child nodes
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                node->program.statements[i] = 
                    substitute_type_params(node->program.statements[i], 
                                        paramNames, typeArgs, count);
            }
            break;
            
        // Add cases for other node types
    }

    return node;
}

/**
 * @brief Specializes generic code for specific types
 * 
 * Performs type-specific optimizations and transformations on the AST
 * based on the concrete types used in template instantiation.
 * 
 * @param node The AST node to specialize
 * @param typeArgs Array of concrete types
 * @param typeArgCount Number of type arguments
 */
void specialize_generic_code(AstNode* node, Type** typeArgs, int typeArgCount) {
    if (!node) return;

    // Specialize operations based on concrete types
    switch (node->type) {
        case AST_BINARY_OP:
            // Add type-specific optimizations
            if (node->binaryOp.op == '+') {
                Type* leftType = infer_type(node->binaryOp.left);
                if (leftType->kind == TYPE_STRING) {
                    // Convert to string concatenation
                    node->type = AST_FUNC_CALL;
                    strncpy(node->funcCall.name, "string_concat",
                           sizeof(node->funcCall.name));
                    // Set up arguments
                    node->funcCall.argCount = 2;
                    node->funcCall.arguments = malloc(2 * sizeof(AstNode*));
                    node->funcCall.arguments[0] = node->binaryOp.left;
                    node->funcCall.arguments[1] = node->binaryOp.right;
                }
            }
            break;
            
        // Add cases for other node types
    }

    // Recursively process children
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                specialize_generic_code(node->program.statements[i], 
                                     typeArgs, typeArgCount);
            }
            break;
            
        // Add cases for other node types
    }
}

/**
 * @brief Optimizes template-instantiated code
 * 
 * Performs template-specific optimizations on the AST, such as:
 * - Converting generic operations to type-specific implementations
 * - Optimizing known template functions
 * - Removing unnecessary type checks
 * 
 * @param node The AST node to optimize
 */
void optimize_template(AstNode* node) {
    if (!node) return;

    // Perform template-specific optimizations
    switch (node->type) {
        case AST_FUNC_CALL:
            // Check for known template functions that can be optimized
            if (strcmp(node->funcCall.name, "swap") == 0) {
                // Convert generic swap to type-specific implementation
                Type* argType = infer_type(node->funcCall.arguments[0]);
                if (argType->kind == TYPE_INT || argType->kind == TYPE_FLOAT) {
                    // Use primitive swap
                    strncpy(node->funcCall.name, 
                           argType->kind == TYPE_INT ? "swap_int" : "swap_float",
                           sizeof(node->funcCall.name));
                }
            }
            break;
            
        // Add cases for other node types
    }

    // Recursively optimize children
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                optimize_template(node->program.statements[i]);
            }
            break;
            
        // Add cases for other node types
    }
}
