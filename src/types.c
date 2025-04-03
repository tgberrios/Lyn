/**
 * @file types.c
 * @brief Implementation of the Lyn compiler's type system
 * 
 * This file implements the core type system functionality, including:
 * - Type creation and management
 * - Type checking and compatibility verification
 * - Type inference
 * - Type system statistics tracking
 * - Debug and logging capabilities
 */

#include "types.h"
#include "ast.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Debug level for the type system
 * 
 * Controls the verbosity of type system logging:
 * - 0: Minimal logging
 * - 1: Basic type system info
 * - 2: Detailed type checking info
 * - 3: Verbose debugging output
 */
static int debug_level = 1;

/**
 * @brief Statistics tracking structure for the type system
 * 
 * Keeps track of various type system metrics:
 * - Number of types created
 * - Number of types freed
 * - Number of type errors detected
 * - Number of classes declared
 * - Number of functions with assigned types
 */
static TypeSystemStats stats = {0};

/**
 * @brief Predefined type definitions
 * 
 * Static type objects for the basic types in the language:
 * - Integer type
 * - Float type
 * - Boolean type
 * - String type
 * - Void type
 * - Unknown type (for type inference)
 */
static Type INTEGER_TYPE = { TYPE_INT, "int" };
static Type FLOAT_TYPE   = { TYPE_FLOAT, "float" };
static Type BOOLEAN_TYPE = { TYPE_BOOL, "bool" };
static Type STRING_TYPE  = { TYPE_STRING, "string" };
static Type VOID_TYPE    = { TYPE_VOID, "void" };
static Type UNKNOWN_TYPE = { TYPE_UNKNOWN, "unknown" };

/**
 * @brief Sets the debug level for the type system
 * 
 * Controls the verbosity of type system logging and error reporting.
 * 
 * @param level New debug level (0-3)
 */
void types_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)types_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Type system debug level set to %d", level);
}

/**
 * @brief Gets the current debug level for the type system
 * 
 * @return int Current debug level (0-3)
 */
int types_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)types_get_debug_level);
    return debug_level;
}

/**
 * @brief Gets the current type system statistics
 * 
 * Returns statistics about type system operations performed during
 * compilation.
 * 
 * @return TypeSystemStats Current type system statistics
 */
TypeSystemStats types_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)types_get_stats);
    return stats;
}

/**
 * @brief Creates a new basic type
 * 
 * Allocates and initializes a new type of the specified kind.
 * Basic types include primitive types like integers, floats, booleans, etc.
 * 
 * @param kind The kind of type to create
 * @return Type* Newly created type, or NULL if allocation fails
 */
Type* createBasicType(TypeKind kind) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)createBasicType);
    
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for basic type", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for basic type");
        return NULL;
    }
    
    type->kind = kind;
    switch(kind) {
        case TYPE_INT:   strcpy(type->typeName, "int"); break;
        case TYPE_FLOAT: strcpy(type->typeName, "float"); break;
        case TYPE_BOOL:  strcpy(type->typeName, "bool"); break;
        case TYPE_STRING: strcpy(type->typeName, "string"); break;
        case TYPE_VOID:  strcpy(type->typeName, "void"); break;
        default:         strcpy(type->typeName, "unknown"); break;
    }
    
    stats.types_created++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Created basic type: %s", type->typeName);
    }
    
    return type;
}

/**
 * @brief Creates a new array type
 * 
 * Allocates and initializes a new array type with the specified element type.
 * Array types are used to represent sequences of values of the same type.
 * 
 * @param elementType The type of elements in the array
 * @return Type* Newly created array type, or NULL if allocation fails
 */
Type* createArrayType(Type* elementType) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)createArrayType);
    
    if (!elementType) {
        logger_log(LOG_WARNING, "Creating array type with NULL element type");
        return NULL;
    }
    
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for array type", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for array type");
        return NULL;
    }
    
    type->kind = TYPE_ARRAY;
    snprintf(type->typeName, sizeof(type->typeName), "[%s]", typeToString(elementType));
    type->arrayType.elementType = elementType;
    
    stats.types_created++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Created array type: %s", type->typeName);
    }
    
    return type;
}

/**
 * @brief Creates a new class type
 * 
 * Allocates and initializes a new class type with the specified name and optional base class.
 * Class types are used to represent user-defined types with their own members and methods.
 * 
 * @param name The name of the class
 * @param baseClass Optional base class for inheritance (can be NULL)
 * @return Type* Newly created class type, or NULL if allocation fails
 */
Type* createClassType(const char* name, Type* baseClass) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)createClassType);
    
    if (!name || !*name) {
        error_report("TypeSystem", __LINE__, 0, "Class type requires a name", ERROR_TYPE);
        logger_log(LOG_ERROR, "Attempted to create class type with empty name");
        return NULL;
    }
    
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for class type", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for class type '%s'", name);
        return NULL;
    }
    
    type->kind = TYPE_CLASS;
    strncpy(type->classType.name, name, sizeof(type->classType.name) - 1);
    type->classType.name[sizeof(type->classType.name)-1] = '\0';
    strcpy(type->typeName, name);
    type->classType.baseClass = baseClass;
    
    stats.types_created++;
    stats.classes_declared++;
    
    if (debug_level >= 1) {
        if (baseClass) {
            logger_log(LOG_DEBUG, "Created class type '%s' inheriting from '%s'", 
                      name, baseClass->classType.name);
        } else {
            logger_log(LOG_DEBUG, "Created class type '%s'", name);
        }
    }
    
    return type;
}

/**
 * @brief Creates a new function type
 * 
 * Allocates and initializes a new function type with the specified return type
 * and parameter types. Function types are used to represent both regular functions
 * and lambda expressions.
 * 
 * @param returnType The type returned by the function
 * @param paramTypes Array of parameter types
 * @param paramCount Number of parameters
 * @return Type* Newly created function type, or NULL if allocation fails
 */
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)createFunctionType);
    
    if (!returnType) {
        error_report("TypeSystem", __LINE__, 0, "Function type requires return type", ERROR_TYPE);
        logger_log(LOG_WARNING, "Creating function type with NULL return type");
        returnType = &UNKNOWN_TYPE;
    }
    
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for function type", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for function type");
        return NULL;
    }
    
    type->kind = TYPE_FUNCTION;
    type->functionType.returnType = returnType;
    type->functionType.paramTypes = paramTypes;
    type->functionType.paramCount = paramCount;
    
    char buffer[256] = "func(";
    for (int i = 0; i < paramCount; i++) {
        if (i > 0) strcat(buffer, ", ");
        const char* paramStr = paramTypes && paramTypes[i] ? typeToString(paramTypes[i]) : "unknown";
        strcat(buffer, paramStr);
    }
    strcat(buffer, ") -> ");
    strcat(buffer, typeToString(returnType));
    strncpy(type->typeName, buffer, sizeof(type->typeName)-1);
    type->typeName[sizeof(type->typeName)-1] = '\0';
    
    stats.types_created++;
    stats.functions_typed++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Created function type: %s", type->typeName);
    }
    
    return type;
}

/**
 * @brief Creates a new curried function type
 * 
 * Allocates and initializes a new curried function type, which represents
 * a partially applied function. Curried functions are used to support
 * functional programming features like partial application.
 * 
 * @param baseType The original function type being curried
 * @param appliedArgCount Number of arguments already applied
 * @return Type* Newly created curried function type, or NULL if allocation fails
 */
Type* create_curried_type(Type* baseType, int appliedArgCount) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)create_curried_type);
    
    if (!baseType || baseType->kind != TYPE_FUNCTION) {
        logger_log(LOG_ERROR, "Cannot curry non-function type");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    if (appliedArgCount >= baseType->functionType.paramCount) {
        // If all arguments are applied, this would be the return type
        return clone_type(baseType->functionType.returnType);
    }
    
    Type* type = malloc(sizeof(Type));
    if (!type) {
        error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for curried type", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for curried type");
        return NULL;
    }
    
    type->kind = TYPE_CURRIED;
    type->curriedType.baseType = clone_type(baseType);
    type->curriedType.appliedArgCount = appliedArgCount;
    
    // Generate the type name
    snprintf(type->typeName, sizeof(type->typeName), "curried(%s, %d)", 
             typeToString(baseType), appliedArgCount);
    
    stats.types_created++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Created curried type for %s with %d arguments applied", 
                  typeToString(baseType), appliedArgCount);
    }
    
    return type;
}

/**
 * @brief Converts a type to its string representation
 * 
 * Generates a human-readable string representation of a type.
 * This is used for debugging, error messages, and type name generation.
 * 
 * @param type The type to convert to string
 * @return const char* String representation of the type
 */
const char* typeToString(Type* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)typeToString);
    
    static char buffer[256];
    
    if (!type) {
        logger_log(LOG_WARNING, "Attempted to convert NULL type to string");
        return "unknown";
    }
    
    switch(type->kind) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_UNKNOWN: return "unknown";
        case TYPE_ARRAY:
            snprintf(buffer, sizeof(buffer), "[%s]", 
                    type->arrayType.elementType ? typeToString(type->arrayType.elementType) : "unknown");
            return buffer;
        case TYPE_CLASS:
            return type->classType.name;
        case TYPE_FUNCTION:
        case TYPE_LAMBDA: {
            char temp[128] = "func(";
            for (int i = 0; i < type->functionType.paramCount; i++) {
                if (i > 0) strcat(temp, ", ");
                if (type->functionType.paramTypes && type->functionType.paramTypes[i]) {
                    strcat(temp, typeToString(type->functionType.paramTypes[i]));
                } else {
                    strcat(temp, "unknown");
                }
            }
            strcat(temp, ") -> ");
            if (type->functionType.returnType) {
                strcat(temp, typeToString(type->functionType.returnType));
            } else {
                strcat(temp, "unknown");
            }
            strncpy(buffer, temp, sizeof(buffer));
            return buffer;
        }
        case TYPE_CURRIED: {
            if (type->curriedType.baseType) {
                snprintf(buffer, sizeof(buffer), "curried(%s, applied=%d)", 
                        typeToString(type->curriedType.baseType),
                        type->curriedType.appliedArgCount);
            } else {
                snprintf(buffer, sizeof(buffer), "curried(unknown)");
            }
            return buffer;
        }
    }
    return "unknown";
}

/**
 * @brief Converts a type to its C language representation
 * 
 * Generates a C language type string representation of a type.
 * This is used for code generation and C interop.
 * 
 * @param type The type to convert
 * @param buffer Buffer to store the C type string
 * @param bufferSize Size of the buffer
 */
void typeToC(Type* type, char* buffer, int bufferSize) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)typeToC);
    
    if (!buffer || bufferSize <= 0) {
        logger_log(LOG_ERROR, "Invalid buffer for typeToC");
        return;
    }
    
    if (!type) {
        logger_log(LOG_WARNING, "Attempted to convert NULL type to C type");
        strncpy(buffer, "void*", bufferSize);
        return;
    }
    
    switch(type->kind) {
        case TYPE_INT:
            strncpy(buffer, "int", bufferSize);
            break;
        case TYPE_FLOAT:
            strncpy(buffer, "float", bufferSize);
            break;
        case TYPE_BOOL:
            strncpy(buffer, "bool", bufferSize);
            break;
        case TYPE_STRING:
            strncpy(buffer, "char*", bufferSize);
            break;
        case TYPE_VOID:
            strncpy(buffer, "void", bufferSize);
            break;
        case TYPE_UNKNOWN:
            strncpy(buffer, "void*", bufferSize);
            break;
        case TYPE_ARRAY: {
            char elementType[128];
            typeToC(type->arrayType.elementType, elementType, sizeof(elementType));
            snprintf(buffer, bufferSize, "%s*", elementType);
            break;
        }
        case TYPE_CLASS:
            snprintf(buffer, bufferSize, "struct %s*", type->classType.name);
            break;
        case TYPE_FUNCTION:
        case TYPE_LAMBDA: {
            char returnType[128];
            typeToC(type->functionType.returnType, returnType, sizeof(returnType));
            snprintf(buffer, bufferSize, "%s (*)(", returnType);
            for (int i = 0; i < type->functionType.paramCount; i++) {
                if (i > 0) strncat(buffer, ", ", bufferSize - strlen(buffer) - 1);
                char paramType[128];
                typeToC(type->functionType.paramTypes[i], paramType, sizeof(paramType));
                strncat(buffer, paramType, bufferSize - strlen(buffer) - 1);
            }
            strncat(buffer, ")", bufferSize - strlen(buffer) - 1);
            break;
        }
        case TYPE_CURRIED: {
            char baseTypeStr[128];
            typeToC(type->curriedType.baseType, baseTypeStr, sizeof(baseTypeStr));
            snprintf(buffer, bufferSize, "%s", baseTypeStr);
            break;
        }
    }
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Converted type '%s' to C type '%s'", typeToString(type), buffer);
    }
}

/**
 * @brief Frees memory allocated for a type
 * 
 * Properly deallocates memory for a type and all its nested types.
 * Handles recursive deallocation for complex types like arrays and functions.
 * 
 * @param type The type to free
 */
void freeType(Type* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)freeType);
    
    if (!type) return;
    
    // Don't free predefined static types
    if (type == &INTEGER_TYPE || type == &FLOAT_TYPE || type == &BOOLEAN_TYPE ||
        type == &STRING_TYPE || type == &VOID_TYPE || type == &UNKNOWN_TYPE) {
        return;
    }
    
    switch (type->kind) {
        case TYPE_ARRAY:
            if (type->arrayType.elementType) {
                freeType(type->arrayType.elementType);
            }
            break;
        case TYPE_CLASS:
            // Don't free baseClass here to avoid recursive freeing
            break;
        case TYPE_FUNCTION:
        case TYPE_LAMBDA:
            if (type->functionType.returnType) {
                freeType(type->functionType.returnType);
            }
            if (type->functionType.paramTypes) {
                for (int i = 0; i < type->functionType.paramCount; i++) {
                    freeType(type->functionType.paramTypes[i]);
                }
                free(type->functionType.paramTypes);
            }
            break;
        case TYPE_CURRIED:
            if (type->curriedType.baseType) {
                freeType(type->curriedType.baseType);
            }
            break;
        default:
            break;
    }
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Freed type: %s", type->typeName);
    }
    
    free(type);
    stats.types_freed++;
}

/**
 * @brief Creates a deep copy of a type
 * 
 * Creates a complete copy of a type and all its nested types.
 * This is used when types need to be modified independently.
 * 
 * @param type The type to clone
 * @return Type* New copy of the type, or NULL if allocation fails
 */
Type* clone_type(Type* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)clone_type);
    
    if (!type) return NULL;
    
    // Return predefined types directly
    if (type == &INTEGER_TYPE) return &INTEGER_TYPE;
    if (type == &FLOAT_TYPE) return &FLOAT_TYPE;
    if (type == &BOOLEAN_TYPE) return &BOOLEAN_TYPE;
    if (type == &STRING_TYPE) return &STRING_TYPE;
    if (type == &VOID_TYPE) return &VOID_TYPE;
    if (type == &UNKNOWN_TYPE) return &UNKNOWN_TYPE;
    
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        case TYPE_STRING:
        case TYPE_VOID:
        case TYPE_UNKNOWN:
            return createBasicType(type->kind);
        case TYPE_ARRAY:
            return createArrayType(clone_type(type->arrayType.elementType));
        case TYPE_CLASS:
            return createClassType(type->classType.name,
                                   type->classType.baseClass ? clone_type(type->classType.baseClass) : NULL);
        case TYPE_FUNCTION:
        case TYPE_LAMBDA: {
            Type** paramTypes = NULL;
            if (type->functionType.paramCount > 0 && type->functionType.paramTypes) {
                paramTypes = malloc(type->functionType.paramCount * sizeof(Type*));
                if (!paramTypes) {
                    error_report("TypeSystem", __LINE__, 0, "Failed to allocate memory for cloned function params", ERROR_MEMORY);
                    return NULL;
                }
                for (int i = 0; i < type->functionType.paramCount; i++) {
                    paramTypes[i] = clone_type(type->functionType.paramTypes[i]);
                }
            }
            return createFunctionType(clone_type(type->functionType.returnType),
                                      paramTypes, type->functionType.paramCount);
        }
        case TYPE_CURRIED:
            return create_curried_type(clone_type(type->curriedType.baseType), type->curriedType.appliedArgCount);
    }
    return NULL;
}

/**
 * @brief Creates a primitive type instance
 * 
 * Returns a reference to a predefined primitive type or creates a new one
 * if the kind is not a primitive type.
 * 
 * @param kind The kind of primitive type to create
 * @return Type* Reference to the primitive type
 */
Type* create_primitive_type(TypeKind kind) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)create_primitive_type);
    
    switch (kind) {
        case TYPE_INT:    return &INTEGER_TYPE;
        case TYPE_FLOAT:  return &FLOAT_TYPE;
        case TYPE_BOOL:   return &BOOLEAN_TYPE;
        case TYPE_STRING: return &STRING_TYPE;
        case TYPE_VOID:   return &VOID_TYPE;
        case TYPE_UNKNOWN:return &UNKNOWN_TYPE;
        default:          
            logger_log(LOG_WARNING, "Requested primitive type for non-primitive kind: %d", kind);
            return createBasicType(kind);
    }
}

/**
 * @brief Converts a TypeKind to its string representation
 * 
 * Returns a human-readable string representation of a TypeKind.
 * Used for debugging and error reporting.
 * 
 * @param kind The TypeKind to convert
 * @return const char* String representation of the type kind
 */
const char* type_kind_to_string(TypeKind kind) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)type_kind_to_string);
    
    switch (kind) {
        case TYPE_INT:     return "int";
        case TYPE_FLOAT:   return "float";
        case TYPE_BOOL:    return "bool";
        case TYPE_STRING:  return "string";
        case TYPE_VOID:    return "void";
        case TYPE_UNKNOWN: return "unknown";
        case TYPE_ARRAY:   return "array";
        case TYPE_CLASS:   return "class";
        case TYPE_FUNCTION:return "function";
        case TYPE_LAMBDA:  return "lambda";
        case TYPE_CURRIED: return "curried_function";
        default:           
            logger_log(LOG_WARNING, "Invalid type kind: %d", kind);
            return "invalid_type";
    }
}

/**
 * @brief Checks if two types are exactly equal
 * 
 * Performs a deep comparison of two types to determine if they are
 * structurally identical. This includes checking all nested types
 * for complex types like arrays and functions.
 * 
 * @param type1 First type to compare
 * @param type2 Second type to compare
 * @return bool True if types are exactly equal, false otherwise
 */
bool are_types_equal(Type* type1, Type* type2) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)are_types_equal);
    
    if (type1 == type2) return true; // Same instance
    
    if (!type1 || !type2) {
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Type equality check with NULL type");
        }
        return false;
    }
    
    if (type1->kind != type2->kind) {
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Types differ in kind: %s vs %s", 
                      type_kind_to_string(type1->kind), type_kind_to_string(type2->kind));
        }
        return false;
    }
    
    switch (type1->kind) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        case TYPE_STRING:
        case TYPE_VOID:
        case TYPE_UNKNOWN:
            return true; // Basic types with same kind are equal
            
        case TYPE_ARRAY:
            return are_types_equal(type1->arrayType.elementType, type2->arrayType.elementType);
            
        case TYPE_CLASS:
            return strcmp(type1->classType.name, type2->classType.name) == 0;
            
        case TYPE_FUNCTION:
        case TYPE_LAMBDA:
            // Return types must match
            if (!are_types_equal(type1->functionType.returnType, type2->functionType.returnType))
                return false;
                
            // Parameter count must match
            if (type1->functionType.paramCount != type2->functionType.paramCount)
                return false;
                
            // Each parameter type must match
            for (int i = 0; i < type1->functionType.paramCount; i++) {
                if (!are_types_equal(type1->functionType.paramTypes[i], 
                                   type2->functionType.paramTypes[i]))
                    return false;
            }
            return true;
        case TYPE_CURRIED:
            return type1->curriedType.appliedArgCount == type2->curriedType.appliedArgCount &&
                   are_types_equal(type1->curriedType.baseType, type2->curriedType.baseType);
    }
    
    return false;
}

/**
 * @brief Checks if a type is a subtype of another type
 * 
 * Implements type inheritance checking, determining if one type
 * can be used where another type is expected. This includes
 * class inheritance and primitive type promotions.
 * 
 * @param type The type to check
 * @param supertype The potential supertype
 * @return bool True if type is a subtype of supertype, false otherwise
 */
bool is_subtype_of(Type* type, Type* supertype) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)is_subtype_of);
    
    if (!type || !supertype) {
        logger_log(LOG_WARNING, "Subtype check with NULL type");
        return false;
    }
    
    // Same type
    if (are_types_equal(type, supertype)) return true;
    
    // Special case: unknown type can be assigned to anything
    if (type->kind == TYPE_UNKNOWN) return true;
    
    // Special case: anything can be assigned to unknown
    if (supertype->kind == TYPE_UNKNOWN) return true;
    
    if (type->kind == TYPE_CLASS && supertype->kind == TYPE_CLASS) {
        // Check class hierarchy
        Type* current = type;
        while (current) {
            if (strcmp(current->classType.name, supertype->classType.name) == 0) {
                if (debug_level >= 2) {
                    logger_log(LOG_DEBUG, "Class '%s' is a subtype of '%s'", 
                              type->classType.name, supertype->classType.name);
                }
                return true;
            }
            current = current->classType.baseClass;
        }
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Class '%s' is NOT a subtype of '%s'", 
                      type->classType.name, supertype->classType.name);
        }
    }
    
    return false;
}

/**
 * @brief Gets the type of a class member
 * 
 * Looks up the type of a member (field or method) in a class type.
 * This includes checking the class hierarchy for inherited members.
 * 
 * @param classType The class type to search in
 * @param memberName The name of the member to look up
 * @return Type* The type of the member, or TYPE_UNKNOWN if not found
 */
Type* get_member_type(Type* classType, const char* memberName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)get_member_type);
    
    if (!classType || !memberName) {
        logger_log(LOG_WARNING, "Null parameter passed to get_member_type");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    if (classType->kind != TYPE_CLASS) {
        logger_log(LOG_WARNING, "Cannot get member from non-class type: %s", typeToString(classType));
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    // In a real compiler, we would lookup the member in the class symbol table
    // For this example, we'll implement some hardcoded cases
    if (strcmp(classType->classType.name, "Point") == 0) {
        if (strcmp(memberName, "x") == 0 || strcmp(memberName, "y") == 0) {
            return create_primitive_type(TYPE_FLOAT);
        }
    } 
    else if (strcmp(classType->classType.name, "Vector3") == 0) {
        if (strcmp(memberName, "x") == 0 || 
            strcmp(memberName, "y") == 0 || 
            strcmp(memberName, "z") == 0) {
            return create_primitive_type(TYPE_FLOAT);
        }
    }
    else if (strcmp(classType->classType.name, "Circle") == 0) {
        if (strcmp(memberName, "radius") == 0) {
            return create_primitive_type(TYPE_FLOAT);
        }
        if (strcmp(memberName, "type") == 0) {
            return create_primitive_type(TYPE_INT);
        }
    }
    
    // If not found in this class, check base class
    if (classType->classType.baseClass) {
        return get_member_type(classType->classType.baseClass, memberName);
    }
    
    return create_primitive_type(TYPE_UNKNOWN);
}

/**
 * @brief Infers the type of an AST node
 * 
 * Recursively infers the type of an AST node and its children.
 * This is the main type inference function that handles all AST node types.
 * The inferred type is cached in the node for future use.
 * 
 * @param node The AST node to infer types for
 * @return Type* The inferred type, or TYPE_UNKNOWN if inference fails
 */
Type* infer_type(struct AstNode* node) {
    if (!node) return create_primitive_type(TYPE_UNKNOWN);
    
    // Cache type if already inferred
    if (node->inferredType) {
        return node->inferredType;
    }
    
    Type* result = NULL;
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            // Check if the value is an integer or float
            if (node->numberLiteral.value == (int)node->numberLiteral.value) {
                result = create_primitive_type(TYPE_INT);
            } else {
                result = create_primitive_type(TYPE_FLOAT);
            }
            break;
            
        case AST_STRING_LITERAL:
            result = create_primitive_type(TYPE_STRING);
            break;
            
        case AST_BINARY_OP:
            {
                Type* left_type = infer_type(node->binaryOp.left);
                Type* right_type = infer_type(node->binaryOp.right);
                
                // Convert operator to string
                char op_str[2] = {node->binaryOp.op, '\0'};
                result = infer_type_from_binary_op(left_type, right_type, op_str);
            }
            break;
            
        case AST_IDENTIFIER:
            {
                // In a real compiler, we would look up the identifier in the symbol table
                // For this example, we'll do some basic hardcoded checks
                if (strcmp(node->identifier.name, "i") == 0 ||
                    strcmp(node->identifier.name, "j") == 0 ||
                    strcmp(node->identifier.name, "k") == 0) {
                    result = create_primitive_type(TYPE_INT);
                } else if (strcmp(node->identifier.name, "pi") == 0 ||
                           strcmp(node->identifier.name, "e") == 0) {
                    result = create_primitive_type(TYPE_FLOAT);
                } else if (strcmp(node->identifier.name, "p1") == 0) {
                    result = createClassType("Point", NULL);
                } else if (strcmp(node->identifier.name, "v1") == 0) {
                    result = createClassType("Vector3", NULL);
                } else if (strcmp(node->identifier.name, "c1") == 0) {
                    result = createClassType("Circle", NULL);
                } else {
                    result = create_primitive_type(TYPE_UNKNOWN);
                }
            }
            break;
            
        case AST_FUNC_CALL:
            {
                // Handle known functions
                if (strcmp(node->funcCall.name, "Point_init") == 0) {
                    result = create_primitive_type(TYPE_VOID);
                } else if (strcmp(node->funcCall.name, "Point_distance") == 0) {
                    result = create_primitive_type(TYPE_FLOAT);
                } else if (strcmp(node->funcCall.name, "Vector3_magnitude") == 0) {
                    result = create_primitive_type(TYPE_FLOAT);
                } else if (strcmp(node->funcCall.name, "Circle_area") == 0) {
                    result = create_primitive_type(TYPE_FLOAT);
                } else if (strcmp(node->funcCall.name, "Circle_scale") == 0) {
                    result = create_primitive_type(TYPE_VOID);
                } else if (strcmp(node->funcCall.name, "new_Point") == 0) {
                    result = createClassType("Point", NULL);
                } else if (strcmp(node->funcCall.name, "new_Vector3") == 0) {
                    result = createClassType("Vector3", NULL);
                } else if (strcmp(node->funcCall.name, "new_Circle") == 0) {
                    result = createClassType("Circle", NULL);
                } else {
                    result = create_primitive_type(TYPE_UNKNOWN);
                }
            }
            break;
            
        case AST_MEMBER_ACCESS:
            {
                Type* objectType = infer_type(node->memberAccess.object);
                
                if (objectType->kind == TYPE_CLASS) {
                    result = get_member_type(objectType, node->memberAccess.member);
                } else {
                    fprintf(stderr, "Type error at line %d: cannot access member of non-class type\n", 
                            node->line);
                    result = create_primitive_type(TYPE_UNKNOWN);
                }
            }
            break;
            
        case AST_ARRAY_LITERAL:
            {
                if (node->arrayLiteral.elementCount > 0) {
                    // Infer element type from first element
                    Type* elementType = infer_type(node->arrayLiteral.elements[0]);
                    
                    // Check that all elements have compatible types
                    for (int i = 1; i < node->arrayLiteral.elementCount; i++) {
                        Type* currentType = infer_type(node->arrayLiteral.elements[i]);
                        if (!types_are_compatible(elementType, currentType)) {
                            fprintf(stderr, "Type error at line %d: array contains incompatible element types\n", 
                                    node->line);
                            elementType = create_primitive_type(TYPE_UNKNOWN);
                            break;
                        }
                    }
                    
                    result = createArrayType(elementType);
                } else {
                    // Empty array, use unknown element type
                    result = createArrayType(create_primitive_type(TYPE_UNKNOWN));
                }
            }
            break;
            
        case AST_LAMBDA:
            {
                // Get return type
                Type* returnType;
                if (strlen(node->lambda.returnType) > 0) {
                    // Parse return type from string
                    if (strcmp(node->lambda.returnType, "int") == 0) {
                        returnType = create_primitive_type(TYPE_INT);
                    } else if (strcmp(node->lambda.returnType, "float") == 0) {
                        returnType = create_primitive_type(TYPE_FLOAT);
                    } else if (strcmp(node->lambda.returnType, "bool") == 0) {
                        returnType = create_primitive_type(TYPE_BOOL);
                    } else if (strcmp(node->lambda.returnType, "string") == 0) {
                        returnType = create_primitive_type(TYPE_STRING);
                    } else if (strcmp(node->lambda.returnType, "void") == 0) {
                        returnType = create_primitive_type(TYPE_VOID);
                    } else {
                        returnType = createClassType(node->lambda.returnType, NULL);
                    }
                } else {
                    // Infer return type from body
                    returnType = infer_type(node->lambda.body);
                }
                
                // Get parameter types
                Type** paramTypes = NULL;
                if (node->lambda.paramCount > 0) {
                    paramTypes = malloc(sizeof(Type*) * node->lambda.paramCount);
                    for (int i = 0; i < node->lambda.paramCount; i++) {
                        // Try to get the type from parameter if available
                        if (node->lambda.parameters[i]->inferredType) {
                            paramTypes[i] = clone_type(node->lambda.parameters[i]->inferredType);
                        } else {
                            paramTypes[i] = create_primitive_type(TYPE_UNKNOWN);
                        }
                    }
                }
                
                result = createFunctionType(returnType, paramTypes, node->lambda.paramCount);
                result->kind = TYPE_LAMBDA;  // Update to lambda kind
            }
            break;
            
        case AST_VAR_DECL:
            {
                // Get type from declaration
                if (strlen(node->varDecl.type) > 0) {
                    if (strcmp(node->varDecl.type, "int") == 0) {
                        result = create_primitive_type(TYPE_INT);
                    } else if (strcmp(node->varDecl.type, "float") == 0) {
                        result = create_primitive_type(TYPE_FLOAT);
                    } else if (strcmp(node->varDecl.type, "bool") == 0) {
                        result = create_primitive_type(TYPE_BOOL);
                    } else if (strcmp(node->varDecl.type, "string") == 0) {
                        result = create_primitive_type(TYPE_STRING);
                    } else if (strcmp(node->varDecl.type, "void") == 0) {
                        result = create_primitive_type(TYPE_VOID);
                    } else {
                        result = createClassType(node->varDecl.type, NULL);
                    }
                } else if (node->varDecl.initializer) {
                    // Infer type from initializer
                    result = infer_type(node->varDecl.initializer);
                } else {
                    result = create_primitive_type(TYPE_UNKNOWN);
                }
            }
            break;
            
        case AST_VAR_ASSIGN:
            {
                if (node->varAssign.initializer) {
                    result = infer_type(node->varAssign.initializer);
                } else {
                    result = create_primitive_type(TYPE_UNKNOWN);
                }
            }
            break;
            
        default:
            result = create_primitive_type(TYPE_UNKNOWN);
            break;
    }
    
    // Cache the result
    node->inferredType = result;
    
    return result;
}

/**
 * @brief Infers the type of a binary operation
 */
Type* infer_type_from_binary_op(Type* left, Type* right, const char* operator) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)infer_type_from_binary_op);
    
    if (!left || !right) {
        logger_log(LOG_WARNING, "Null type in binary operation inference");
        return create_primitive_type(TYPE_UNKNOWN);
    }
    
    if (strcmp(operator, "+") == 0) {
        // String concatenation
        if (left->kind == TYPE_STRING || right->kind == TYPE_STRING) {
            logger_log(LOG_DEBUG, "Inferred string type for concatenation");
            return create_primitive_type(TYPE_STRING);
        }
        
        // Numeric addition
        if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT) {
            return create_primitive_type(TYPE_FLOAT);
        }
        return create_primitive_type(TYPE_INT);
    }
    else if (strcmp(operator, "-") == 0 || 
             strcmp(operator, "*") == 0 || 
             strcmp(operator, "/") == 0) {
        // Numeric operations
        if ((left->kind == TYPE_INT || left->kind == TYPE_FLOAT) &&
            (right->kind == TYPE_INT || right->kind == TYPE_FLOAT)) {
            if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT) {
                return create_primitive_type(TYPE_FLOAT);
            }
            return create_primitive_type(TYPE_INT);
        }
        
        logger_log(LOG_WARNING, "Invalid operands for arithmetic operation: %s", operator);
        return create_primitive_type(TYPE_UNKNOWN);
    }
    else if (strcmp(operator, ">") == 0 || 
             strcmp(operator, "<") == 0 ||
             strcmp(operator, ">=") == 0 || 
             strcmp(operator, "<=") == 0 ||
             strcmp(operator, "==") == 0 || 
             strcmp(operator, "!=") == 0) {
        // Comparison operators always return boolean
        logger_log(LOG_DEBUG, "Inferred boolean type for comparison operation");
        return create_primitive_type(TYPE_BOOL);
    }
    else if (strcmp(operator, "and") == 0 || 
             strcmp(operator, "or") == 0) {
        // Logical operators require boolean operands and return boolean
        if (left->kind != TYPE_BOOL || right->kind != TYPE_BOOL) {
            logger_log(LOG_WARNING, "Logical operators expect boolean operands");
        }
        return create_primitive_type(TYPE_BOOL);
    }
    
    logger_log(LOG_WARNING, "Unrecognized binary operator: %s", operator);
    return create_primitive_type(TYPE_UNKNOWN);
}

/**
 * @brief Checks if two types are compatible (one can be assigned to the other)
 */
bool types_are_compatible(Type* type1, Type* type2) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)types_are_compatible);
    
    if (!type1 || !type2) {
        logger_log(LOG_WARNING, "Type compatibility check with NULL type");
        return false;
    }
    
    // Same type
    if (are_types_equal(type1, type2)) return true;
    
    // Check subtype relationship
    if (is_subtype_of(type1, type2) || is_subtype_of(type2, type1)) return true;
    
    // Special compatibility rules
    if ((type1->kind == TYPE_INT && type2->kind == TYPE_FLOAT) ||
        (type1->kind == TYPE_FLOAT && type2->kind == TYPE_INT)) {
        return true;  // Integer and float are compatible
    }
    
    // String concatenation with numbers
    if (type1->kind == TYPE_STRING && 
        (type2->kind == TYPE_INT || type2->kind == TYPE_FLOAT)) {
        logger_log(LOG_DEBUG, "Compatible: number can be concatenated with string");
        return true;
    }
    
    // Array compatibility requires same element type
    if (type1->kind == TYPE_ARRAY && type2->kind == TYPE_ARRAY) {
        return types_are_compatible(type1->arrayType.elementType, 
                                   type2->arrayType.elementType);
    }
    
    // Function compatibility
    if ((type1->kind == TYPE_FUNCTION || type1->kind == TYPE_LAMBDA) &&
        (type2->kind == TYPE_FUNCTION || type2->kind == TYPE_LAMBDA)) {
        
        // Return types must be compatible
        if (!types_are_compatible(type1->functionType.returnType, 
                                  type2->functionType.returnType)) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Function return types incompatible: %s vs %s", 
                          typeToString(type1->functionType.returnType),
                          typeToString(type2->functionType.returnType));
            }
            return false;
        }
        
        // Parameter count must match
        if (type1->functionType.paramCount != type2->functionType.paramCount) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Function parameter counts differ: %d vs %d", 
                          type1->functionType.paramCount, type2->functionType.paramCount);
            }
            return false;
        }
        
        // Each parameter type must be compatible
        for (int i = 0; i < type1->functionType.paramCount; i++) {
            if (!types_are_compatible(type1->functionType.paramTypes[i],
                                      type2->functionType.paramTypes[i])) {
                return false;
            }
        }
        return true;
    }
    
    return false;
}
