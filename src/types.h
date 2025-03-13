#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>  // Added for bool type

// Forward declaration
struct AstNode;

// Enumerador para los tipos básicos y compuestos
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_UNKNOWN,
    TYPE_ARRAY,
    TYPE_CLASS,
    TYPE_FUNCTION,
    TYPE_LAMBDA
} TypeKind;

typedef struct Type {
    TypeKind kind;
    char typeName[64];  // Campo para almacenar el nombre del tipo en forma de cadena
    union {
        // Tipo array: apunta al tipo de sus elementos
        struct {
            struct Type* elementType;
        } arrayType;
        // Tipo clase: nombre y (opcional) clase base
        struct {
            char name[64];
            struct Type* baseClass;
        } classType;
        // Tipo función (y lambda): tipo de retorno, arreglo de tipos de parámetros y cantidad
        struct {
            struct Type* returnType;
            struct Type** paramTypes;
            int paramCount;
        } functionType;
    };
} Type;

// Prototipos de funciones para trabajar con tipos
Type* createBasicType(TypeKind kind);
Type* createArrayType(Type* elementType);
Type* createClassType(const char* name, Type* baseClass);
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount);
Type* clone_type(Type* type);
const char* typeToString(Type* type);
void typeToC(Type* type, char* buffer, int bufferSize);
void freeType(Type* type);

// Funciones "wrapper" para tipos primitivos y compuestos
Type* create_primitive_type(TypeKind kind);
Type* create_function_type(Type* returnType, Type** paramTypes, int paramCount);
Type* create_class_type(const char* name, Type* baseClass);

// Type checking and comparison functions
bool are_types_compatible(Type* type1, Type* type2);
bool is_subtype_of(Type* type, Type* supertype);
bool are_types_equal(Type* type1, Type* type2);

// Type inference and checking
// These functions will be defined in types.c, not in the header
// to avoid circular dependencies
Type* infer_type(struct AstNode* node);
bool check_ast_types(struct AstNode* node);
void check_types(struct AstNode* node, Type* expected);

// Type system utilities
const char* type_kind_to_string(TypeKind kind);
Type* get_member_type(Type* classType, const char* memberName);

#endif
