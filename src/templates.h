/**
 * @file templates.h
 * @brief Header file for the template system in the Lyn compiler
 * 
 * This header defines the interface for the template system that enables
 * generic programming in the Lyn language. It provides:
 * - Template parameter and definition structures
 * - Template instantiation support
 * - AST manipulation utilities for template processing
 */

#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ast.h"
#include "types.h"

/**
 * @brief Structure representing a template parameter
 * 
 * Contains information about a template parameter, including its name
 * and optional type constraint.
 */
typedef struct {
    /** Name of the template parameter */
    char name[256];
    /** Type constraint for the parameter (NULL if no constraint) */
    Type* constraint;
} TemplateParam;

/**
 * @brief Structure representing a template definition
 * 
 * Contains the complete definition of a template, including its name,
 * parameters, and body.
 */
typedef struct {
    /** Name of the template */
    char name[256];
    /** Array of template parameters */
    TemplateParam** params;
    /** Number of parameters */
    int paramCount;
    /** AST node representing the template body */
    AstNode* body;
} TemplateDefinition;

/**
 * @brief Structure representing a template instantiation
 * 
 * Contains information about a specific instantiation of a template,
 * including the template name and type arguments.
 */
typedef struct {
    /** Name of the template being instantiated */
    const char* templateName;
    /** Array of type arguments */
    Type** typeArgs;
    /** Number of type arguments */
    int typeArgCount;
} TemplateInstance;

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
bool register_template(const char* name, TemplateParam** params, int paramCount, AstNode* body);

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
AstNode* instantiate_template(const char* name, Type** typeArgs, int typeArgCount);

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
void optimize_template(AstNode* node);

/**
 * @brief Validates type arguments against template constraints
 * 
 * Checks if each type argument satisfies its corresponding template parameter
 * constraint. This ensures type safety during template instantiation.
 * 
 * @param instance The template instance to validate
 * @return bool true if all constraints are satisfied, false otherwise
 */
bool validate_template_constraints(TemplateInstance* instance);

/**
 * @brief Creates a deep copy of an AST node
 * 
 * Recursively clones an AST node and all its children, including
 * type information and node-specific data.
 * 
 * @param node The AST node to clone
 * @return AstNode* A deep copy of the node, or NULL if input is NULL
 */
AstNode* clone_ast_node(AstNode* node);

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
AstNode* substitute_type_params(AstNode* node, const char** paramNames, Type** typeArgs, int count);

/**
 * @brief Inlines template function calls in the AST
 * 
 * Replaces template function calls with their expanded forms by
 * substituting type parameters and inlining the function body.
 * 
 * @param node The AST node to process
 * @return AstNode* Modified AST with template calls inlined
 */
AstNode* inline_template_calls(AstNode* node);

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
void specialize_generic_code(AstNode* node, Type** typeArgs, int typeArgCount);

#endif
