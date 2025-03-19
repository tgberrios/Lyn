#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include "error.h"
#include "logger.h"

// Forward declarations
struct AstNode;

// First define the TypeKind enum
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
    TYPE_LAMBDA,
    TYPE_CURRIED     // New type for curried functions
} TypeKind;

// Then define the Type structure
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
        // Tipo curried function: tipo base y argumentos ya aplicados
        struct {
            struct Type* baseType;      // Base function type
            int appliedArgCount;        // Number of arguments already applied
        } curriedType;
    };
} Type;

// Now we can define the reflection structures that use Type
typedef struct {
    char name[256];
    Type* type;
    int modifiers;  // public, private, etc.
} FieldInfo;

typedef struct {
    char name[256];
    Type* returnType;
    Type** paramTypes;
    int paramCount;
    char** paramNames;
    int modifiers;
} MethodInfo;

typedef struct {
    char name[256];
    Type* type;
    FieldInfo** fields;
    int fieldCount;
    MethodInfo** methods;
    int methodCount;
    Type* baseType;
    bool isBuiltin;
} TypeInfo;

// Configuración del nivel de depuración del sistema de tipos
void types_set_debug_level(int level);
int types_get_debug_level(void);

// Estadísticas sobre el sistema de tipos
typedef struct {
    int types_created;         // Cuántos tipos se han creado
    int types_freed;           // Cuántos tipos se han liberado
    int type_errors_detected;  // Cuántos errores de tipo se han detectado
    int classes_declared;      // Cuántas clases se han declarado
    int functions_typed;       // Cuántas funciones tienen tipo asignado
} TypeSystemStats;

// Obtener estadísticas del sistema de tipos
TypeSystemStats types_get_stats(void);

// Prototipos de funciones para trabajar con tipos
Type* createBasicType(TypeKind kind);
Type* createArrayType(Type* elementType);
Type* createClassType(const char* name, Type* baseClass);
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount);
Type* create_curried_type(Type* baseType, int appliedArgCount);
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
Type* infer_type(struct AstNode* node);
bool check_ast_types(struct AstNode* node);
void check_types(struct AstNode* node, Type* expected);

// Type system utilities
const char* type_kind_to_string(TypeKind kind);
Type* get_member_type(Type* classType, const char* memberName);

// Type introspection functions
const char* typeof_value(struct AstNode* expr);
TypeInfo* get_type_info(Type* type);
FieldInfo** get_fields(Type* type, int* count);
MethodInfo** get_methods(Type* type, int* count);

#endif
