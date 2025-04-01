/**
 * @file reflection.h
 * @brief Header file for the runtime reflection system
 * 
 * This header defines the interface for the reflection system that enables
 * runtime inspection and manipulation of types and objects. It provides:
 * - Runtime type information (RTTI)
 * - Object introspection
 * - Dynamic method invocation
 * - Field access and modification
 * - Type compatibility checking
 * - Interface implementation verification
 */

#ifndef REFLECTION_H
#define REFLECTION_H

#include "types.h"
#include "ast.h"

/**
 * @brief Structure containing runtime type information
 * 
 * This structure holds all the information needed for runtime reflection,
 * including type information, virtual method table, and additional metadata.
 */
typedef struct RuntimeType {
    TypeInfo* typeInfo;  ///< Basic type information
    void* vtable;        ///< Virtual method table for dynamic dispatch
    void* metadata;      ///< Additional type-specific metadata
} RuntimeType;

/**
 * @brief Gets runtime type information for an expression
 * 
 * @param expr The AST expression to get type information for
 * @return RuntimeType* Runtime type information, or NULL if expr is NULL
 */
RuntimeType* get_runtime_type(AstNode* expr);

/**
 * @brief Checks if an expression is an instance of a given type
 * 
 * @param expr The expression to check
 * @param type The type to check against
 * @return bool true if expr is compatible with type, false otherwise
 */
bool is_instance_of(AstNode* expr, Type* type);

/**
 * @brief Creates a new instance of a type
 * 
 * @param type The type to create an instance of
 * @return AstNode* The newly created instance, or NULL on error
 */
AstNode* create_instance(Type* type);

/**
 * @brief Sets the value of a field on an object
 * 
 * @param obj The object to modify
 * @param fieldName The name of the field to set
 * @param value The new value to set
 * @return bool true if successful, false if field doesn't exist or is read-only
 */
bool set_field(AstNode* obj, const char* fieldName, AstNode* value);

/**
 * @brief Gets the value of a field from an object
 * 
 * @param obj The object to read from
 * @param fieldName The name of the field to get
 * @return AstNode* The field value, or NULL if field doesn't exist
 */
AstNode* get_field(AstNode* obj, const char* fieldName);

/**
 * @brief Invokes a method on an object
 * 
 * @param obj The object to invoke the method on
 * @param methodName The name of the method to invoke
 * @param args Array of argument nodes
 * @param argCount Number of arguments
 * @return AstNode* The method's return value, or NULL on error
 */
AstNode* invoke_method(AstNode* obj, const char* methodName, AstNode** args, int argCount);

/**
 * @brief Checks if an object has a method
 * 
 * @param obj The object to check
 * @param methodName The name of the method to look for
 * @return bool true if the method exists, false otherwise
 */
bool has_method(AstNode* obj, const char* methodName);

/**
 * @brief Checks if an object has a field
 * 
 * @param obj The object to check
 * @param fieldName The name of the field to look for
 * @return bool true if the field exists, false otherwise
 */
bool has_field(AstNode* obj, const char* fieldName);

/**
 * @brief Checks if one type is a subtype of another
 * 
 * @param type1 The type to check
 * @param type2 The potential supertype
 * @return bool true if type1 is a subtype of type2, false otherwise
 */
bool is_subtype(Type* type1, Type* type2);

/**
 * @brief Checks if a type is an interface
 * 
 * @param type The type to check
 * @return bool true if type is an interface, false otherwise
 */
bool is_interface(Type* type);

/**
 * @brief Checks if a type implements an interface
 * 
 * @param type The type to check
 * @param interface The interface to check against
 * @return bool true if type implements interface, false otherwise
 */
bool implements_interface(Type* type, Type* interface);

/**
 * @brief Converts a type to its string representation
 * 
 * @param type The type to convert
 * @return const char* String representation of the type
 */
const char* type_to_string(Type* type);

/**
 * @brief Prints detailed type information for debugging
 * 
 * @param info The type information to print
 */
void print_type_info(TypeInfo* info);

/**
 * @brief Gets type information by type name
 * 
 * @param name The name of the type to look up
 * @return TypeInfo* Type information if found, NULL otherwise
 */
TypeInfo* get_type_info_by_name(const char* name);

#endif /* REFLECTION_H */
