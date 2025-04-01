/**
 * @file types.h
 * @brief Header file for the Lyn compiler's type system
 * 
 * This file defines the core type system structures and functions for the Lyn compiler.
 * It includes type definitions, type checking, type inference, and type system utilities.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include "error.h"
#include "logger.h"

// Forward declarations
struct AstNode;

/**
 * @brief Enumeration of all possible types in the language
 * 
 * Defines the fundamental types supported by the Lyn compiler:
 * - Basic types: int, float, bool, string, void
 * - Special types: unknown (for type inference), null
 * - Complex types: array, class, function, lambda, curried function
 * - Object type for general object-oriented features
 */
typedef enum {
    TYPE_INT,      ///< Integer type
    TYPE_FLOAT,    ///< Floating-point type
    TYPE_BOOL,     ///< Boolean type
    TYPE_STRING,   ///< String type
    TYPE_VOID,     ///< Void type (no value)
    TYPE_UNKNOWN,  ///< Unknown type (used during type inference)
    TYPE_ARRAY,    ///< Array type
    TYPE_CLASS,    ///< Class type
    TYPE_FUNCTION, ///< Function type
    TYPE_LAMBDA,   ///< Lambda expression type
    TYPE_CURRIED,  ///< Curried function type
    TYPE_OBJECT,   ///< General object type
    TYPE_NULL      ///< Null type
} TypeKind;

/**
 * @brief Core type structure representing all types in the language
 * 
 * The Type structure uses a discriminated union to represent different kinds of types.
 * Each type has a kind and a string name, plus type-specific data in the union.
 */
typedef struct Type {
    TypeKind kind;           ///< The kind of type
    char typeName[64];       ///< String representation of the type name
    
    union {
        /**
         * @brief Array type information
         * 
         * Contains the element type of the array
         */
        struct {
            struct Type* elementType;  ///< Type of array elements
        } arrayType;
        
        /**
         * @brief Class type information
         * 
         * Contains the class name and optional base class
         */
        struct {
            char name[64];            ///< Name of the class
            struct Type* baseClass;   ///< Base class (for inheritance)
        } classType;
        
        /**
         * @brief Function type information
         * 
         * Contains return type, parameter types, and parameter count
         */
        struct {
            struct Type* returnType;   ///< Type returned by the function
            struct Type** paramTypes;  ///< Array of parameter types
            int paramCount;            ///< Number of parameters
        } functionType;
        
        /**
         * @brief Curried function type information
         * 
         * Contains the base function type and number of arguments applied
         */
        struct {
            struct Type* baseType;     ///< Original function type
            int appliedArgCount;       ///< Number of arguments already applied
        } curriedType;
    };
} Type;

/**
 * @brief Structure representing a field in a class or type
 * 
 * Contains information about a field's name, type, and access modifiers.
 * Used for reflection and type introspection.
 */
typedef struct {
    char name[256];     ///< Name of the field
    Type* type;         ///< Type of the field
    int modifiers;      ///< Access modifiers (public, private, etc.)
} FieldInfo;

/**
 * @brief Structure representing a method in a class or type
 * 
 * Contains information about a method's name, return type, parameters,
 * and access modifiers. Used for reflection and type introspection.
 */
typedef struct {
    char name[256];           ///< Name of the method
    Type* returnType;         ///< Return type of the method
    Type** paramTypes;        ///< Array of parameter types
    int paramCount;           ///< Number of parameters
    char** paramNames;        ///< Array of parameter names
    int modifiers;            ///< Access modifiers (public, private, etc.)
} MethodInfo;

/**
 * @brief Structure containing complete type information for reflection
 * 
 * Provides comprehensive information about a type, including its fields,
 * methods, and inheritance relationships. Used for runtime type information
 * and reflection capabilities.
 */
typedef struct {
    char name[256];           ///< Name of the type
    Type* type;               ///< The type itself
    FieldInfo** fields;       ///< Array of field information
    int fieldCount;           ///< Number of fields
    MethodInfo** methods;     ///< Array of method information
    int methodCount;          ///< Number of methods
    Type* baseType;           ///< Base type (for inheritance)
    bool isBuiltin;           ///< Whether this is a built-in type
} TypeInfo;

/**
 * @brief Structure for tracking type system statistics
 * 
 * Maintains counters for various type system operations and states.
 * Used for debugging and performance monitoring.
 */
typedef struct {
    int types_created;         ///< Number of types created
    int types_freed;           ///< Number of types freed
    int type_errors_detected;  ///< Number of type errors detected
    int classes_declared;      ///< Number of classes declared
    int functions_typed;       ///< Number of functions with assigned types
} TypeSystemStats;

/**
 * @brief Sets the debug level for the type system
 * 
 * @param level The new debug level to set
 */
void types_set_debug_level(int level);

/**
 * @brief Gets the current debug level of the type system
 * 
 * @return The current debug level
 */
int types_get_debug_level(void);

/**
 * @brief Retrieves current statistics about the type system
 * 
 * @return Current TypeSystemStats containing various counters
 */
TypeSystemStats types_get_stats(void);

/**
 * @brief Creates a basic type with the specified kind
 * 
 * @param kind The TypeKind to create
 * @return A new Type structure for the basic type
 */
Type* createBasicType(TypeKind kind);

/**
 * @brief Creates an array type with the specified element type
 * 
 * @param elementType The type of elements in the array
 * @return A new Type structure for the array type
 */
Type* createArrayType(Type* elementType);

/**
 * @brief Creates a class type with the specified name and optional base class
 * 
 * @param name The name of the class
 * @param baseClass Optional base class for inheritance
 * @return A new Type structure for the class type
 */
Type* createClassType(const char* name, Type* baseClass);

/**
 * @brief Creates a function type with the specified return type and parameters
 * 
 * @param returnType The type returned by the function
 * @param paramTypes Array of parameter types
 * @param paramCount Number of parameters
 * @return A new Type structure for the function type
 */
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount);

/**
 * @brief Creates a curried function type from a base function type
 * 
 * @param baseType The original function type
 * @param appliedArgCount Number of arguments already applied
 * @return A new Type structure for the curried function type
 */
Type* create_curried_type(Type* baseType, int appliedArgCount);

/**
 * @brief Creates a deep copy of a type and all its nested types
 * 
 * @param type The type to clone
 * @return A new Type structure that is a deep copy of the input type
 */
Type* clone_type(Type* type);

/**
 * @brief Converts a type to its string representation
 * 
 * @param type The type to convert
 * @return A string representation of the type
 */
const char* typeToString(Type* type);

/**
 * @brief Converts a type to its C language representation
 * 
 * @param type The type to convert
 * @param buffer Buffer to store the C representation
 * @param bufferSize Size of the buffer
 */
void typeToC(Type* type, char* buffer, int bufferSize);

/**
 * @brief Frees memory allocated for a type and its nested types
 * 
 * @param type The type to free
 */
void freeType(Type* type);

/**
 * @brief Creates a primitive type with the specified kind
 * 
 * @param kind The TypeKind to create
 * @return A new Type structure for the primitive type
 */
Type* create_primitive_type(TypeKind kind);

/**
 * @brief Creates a function type (wrapper for createFunctionType)
 * 
 * @param returnType The type returned by the function
 * @param paramTypes Array of parameter types
 * @param paramCount Number of parameters
 * @return A new Type structure for the function type
 */
Type* create_function_type(Type* returnType, Type** paramTypes, int paramCount);

/**
 * @brief Creates a class type (wrapper for createClassType)
 * 
 * @param name The name of the class
 * @param baseClass Optional base class for inheritance
 * @return A new Type structure for the class type
 */
Type* create_class_type(const char* name, Type* baseClass);

// Type checking and comparison functions
/**
 * @brief Checks if two types are compatible for assignment or operation
 * 
 * @param type1 First type to compare
 * @param type2 Second type to compare
 * @return true if types are compatible, false otherwise
 */
bool are_types_compatible(Type* type1, Type* type2);

/**
 * @brief Checks if one type is a subtype of another
 * 
 * @param type The type to check
 * @param supertype The potential supertype
 * @return true if type is a subtype of supertype, false otherwise
 */
bool is_subtype_of(Type* type, Type* supertype);

/**
 * @brief Checks if two types are exactly equal
 * 
 * @param type1 First type to compare
 * @param type2 Second type to compare
 * @return true if types are exactly equal, false otherwise
 */
bool are_types_equal(Type* type1, Type* type2);

// Type inference and checking
/**
 * @brief Infers the type of an AST node
 * 
 * @param node The AST node to infer type for
 * @return The inferred type, or NULL if type cannot be inferred
 */
Type* infer_type(struct AstNode* node);

/**
 * @brief Checks types throughout an AST
 * 
 * @param node The root AST node to check
 * @return true if all types are valid, false otherwise
 */
bool check_ast_types(struct AstNode* node);

/**
 * @brief Checks if a node's type matches an expected type
 * 
 * @param node The AST node to check
 * @param expected The expected type
 */
void check_types(struct AstNode* node, Type* expected);

// Type system utilities
/**
 * @brief Converts a TypeKind to its string representation
 * 
 * @param kind The TypeKind to convert
 * @return String representation of the type kind
 */
const char* type_kind_to_string(TypeKind kind);

/**
 * @brief Gets the type of a member in a class type
 * 
 * @param classType The class type to search in
 * @param memberName The name of the member to find
 * @return The type of the member, or NULL if not found
 */
Type* get_member_type(Type* classType, const char* memberName);

// Type introspection functions
/**
 * @brief Gets the type of a value expression
 * 
 * @param expr The expression to get type for
 * @return String representation of the type
 */
const char* typeof_value(struct AstNode* expr);

/**
 * @brief Gets complete type information for a type
 * 
 * @param type The type to get information for
 * @return TypeInfo structure containing complete type information
 */
TypeInfo* get_type_info(Type* type);

/**
 * @brief Gets all fields of a type
 * 
 * @param type The type to get fields for
 * @param count Pointer to store the number of fields
 * @return Array of FieldInfo structures
 */
FieldInfo** get_fields(Type* type, int* count);

/**
 * @brief Gets all methods of a type
 * 
 * @param type The type to get methods for
 * @param count Pointer to store the number of methods
 * @return Array of MethodInfo structures
 */
MethodInfo** get_methods(Type* type, int* count);

// Type inference functions
/**
 * @brief Infers type from a literal value
 * 
 * @param literal The literal string to infer type from
 * @return The inferred type
 */
Type* infer_type_from_literal(const char* literal);

/**
 * @brief Infers type from a binary operation
 * 
 * @param left Left operand type
 * @param right Right operand type
 * @param operator The operator being used
 * @return The inferred result type
 */
Type* infer_type_from_binary_op(Type* left, Type* right, const char* operator);

/**
 * @brief Gets the common type between two types
 * 
 * @param type1 First type
 * @param type2 Second type
 * @return The common type that both types can be promoted to
 */
Type* get_common_type(Type* type1, Type* type2);

// Type checking functions
/**
 * @brief Checks if two types are compatible for assignment
 * 
 * @param expected The expected type
 * @param actual The actual type
 * @return true if types are compatible, false otherwise
 */
bool types_are_compatible(Type* expected, Type* actual);

/**
 * @brief Converts a type to its string representation
 * 
 * @param type The type to convert
 * @return String representation of the type
 */
const char* type_to_string(Type* type);

#endif
