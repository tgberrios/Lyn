#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definiciones estáticas para los tipos predefinidos
static Type INTEGER_TYPE = { TYPE_INT, "int" };
static Type FLOAT_TYPE   = { TYPE_FLOAT, "float" };
static Type BOOLEAN_TYPE = { TYPE_BOOL, "bool" };
static Type STRING_TYPE  = { TYPE_STRING, "string" };
static Type VOID_TYPE    = { TYPE_VOID, "void" };
static Type UNKNOWN_TYPE = { TYPE_UNKNOWN, "unknown" };

Type* createBasicType(TypeKind kind) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el tipo\n");
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
    return type;
}

Type* createArrayType(Type* elementType) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el tipo array\n");
        return NULL;
    }
    type->kind = TYPE_ARRAY;
    snprintf(type->typeName, sizeof(type->typeName), "[%s]", typeToString(elementType));
    type->arrayType.elementType = elementType;
    return type;
}

Type* createClassType(const char* name, Type* baseClass) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el tipo clase\n");
        return NULL;
    }
    type->kind = TYPE_CLASS;
    strncpy(type->classType.name, name, sizeof(type->classType.name) - 1);
    type->classType.name[sizeof(type->classType.name)-1] = '\0';
    // Usamos el nombre de la clase como representación en C
    strcpy(type->typeName, name);
    type->classType.baseClass = baseClass;
    return type;
}

Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        fprintf(stderr, "Error: no se pudo asignar memoria para el tipo función\n");
        return NULL;
    }
    type->kind = TYPE_FUNCTION;
    type->functionType.returnType = returnType;
    type->functionType.paramTypes = paramTypes;
    type->functionType.paramCount = paramCount;
    char buffer[256] = "func(";
    for (int i = 0; i < paramCount; i++) {
        if (i > 0) strcat(buffer, ", ");
        strcat(buffer, typeToString(paramTypes[i]));
    }
    strcat(buffer, ") -> ");
    strcat(buffer, typeToString(returnType));
    strncpy(type->typeName, buffer, sizeof(type->typeName)-1);
    type->typeName[sizeof(type->typeName)-1] = '\0';
    return type;
}

const char* typeToString(Type* type) {
    static char buffer[256];
    if (!type) return "unknown";
    switch(type->kind) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_UNKNOWN: return "unknown";
        case TYPE_ARRAY:
            snprintf(buffer, sizeof(buffer), "[%s]", typeToString(type->arrayType.elementType));
            return buffer;
        case TYPE_CLASS:
            return type->classType.name;
        case TYPE_FUNCTION:
        case TYPE_LAMBDA: {
            char temp[128] = "func(";
            for (int i = 0; i < type->functionType.paramCount; i++) {
                if (i > 0) strcat(temp, ", ");
                strcat(temp, typeToString(type->functionType.paramTypes[i]));
            }
            strcat(temp, ") -> ");
            strcat(temp, typeToString(type->functionType.returnType));
            strncpy(buffer, temp, sizeof(buffer));
            return buffer;
        }
    }
    return "unknown";
}

void typeToC(Type* type, char* buffer, int bufferSize) {
    if (!type) {
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
                if (i > 0) {
                    strncat(buffer, ", ", bufferSize - strlen(buffer) - 1);
                }
                char paramType[128];
                typeToC(type->functionType.paramTypes[i], paramType, sizeof(paramType));
                strncat(buffer, paramType, bufferSize - strlen(buffer) - 1);
            }
            strncat(buffer, ")", bufferSize - strlen(buffer) - 1);
            break;
        }
    }
}

void freeType(Type* type) {
    if (!type) return;
    switch (type->kind) {
        case TYPE_ARRAY:
            freeType(type->arrayType.elementType);
            break;
        case TYPE_CLASS:
            // No liberamos baseClass aquí
            break;
        case TYPE_FUNCTION:
        case TYPE_LAMBDA:
            freeType(type->functionType.returnType);
            for (int i = 0; i < type->functionType.paramCount; i++) {
                freeType(type->functionType.paramTypes[i]);
            }
            free(type->functionType.paramTypes);
            break;
        default:
            break;
    }
    free(type);
}

Type* clone_type(Type* type) {
    if (!type) return NULL;
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
            if (type->functionType.paramCount > 0) {
                paramTypes = malloc(type->functionType.paramCount * sizeof(Type*));
                for (int i = 0; i < type->functionType.paramCount; i++) {
                    paramTypes[i] = clone_type(type->functionType.paramTypes[i]);
                }
            }
            return createFunctionType(clone_type(type->functionType.returnType),
                                      paramTypes, type->functionType.paramCount);
        }
    }
    return NULL;
}

Type* create_primitive_type(TypeKind kind) {
    switch (kind) {
        case TYPE_INT:    return &INTEGER_TYPE;
        case TYPE_FLOAT:  return &FLOAT_TYPE;
        case TYPE_BOOL:   return &BOOLEAN_TYPE;
        case TYPE_STRING: return &STRING_TYPE;
        case TYPE_VOID:   return &VOID_TYPE;
        case TYPE_UNKNOWN:return &UNKNOWN_TYPE;
        default:          return createBasicType(kind);
    }
}

Type* create_function_type(Type* returnType, Type** paramTypes, int paramCount) {
    return createFunctionType(returnType, paramTypes, paramCount);
}

Type* create_class_type(const char* name, Type* baseClass) {
    return createClassType(name, baseClass);
}
