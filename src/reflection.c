#include "reflection.h"
#include "error.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Cache for type information
#define MAX_CACHED_TYPES 1024
static TypeInfo* typeCache[MAX_CACHED_TYPES];
static int typeCacheCount = 0;

RuntimeType* get_runtime_type(AstNode* expr) {
    if (!expr) return NULL;

    RuntimeType* rt = malloc(sizeof(RuntimeType));
    rt->typeInfo = get_type_info(expr->inferredType);
    rt->vtable = NULL;  // To be implemented
    rt->metadata = NULL; // To be implemented

    return rt;
}

TypeInfo* get_type_info(Type* type) {
    if (!type) return NULL;

    // Check cache first
    for (int i = 0; i < typeCacheCount; i++) {
        if (are_types_equal(typeCache[i]->type, type)) {
            return typeCache[i];
        }
    }

    // Create new type info
    TypeInfo* info = malloc(sizeof(TypeInfo));
    strncpy(info->name, typeToString(type), sizeof(info->name) - 1);
    info->type = clone_type(type);
    info->fields = NULL;
    info->fieldCount = 0;
    info->methods = NULL;
    info->methodCount = 0;
    info->baseType = NULL;
    info->isBuiltin = (type->kind <= TYPE_VOID);

    // Add fields and methods based on type kind
    switch (type->kind) {
        case TYPE_CLASS:
            // Get fields
            info->fields = get_fields(type, &info->fieldCount);
            // Get methods
            info->methods = get_methods(type, &info->methodCount);
            // Get base class
            info->baseType = type->classType.baseClass;
            break;
        
        // Add other type kinds as needed
        default:
            break;
    }

    // Cache the type info
    if (typeCacheCount < MAX_CACHED_TYPES) {
        typeCache[typeCacheCount++] = info;
    }

    return info;
}

FieldInfo** get_fields(Type* type, int* count) {
    if (!type || !count) return NULL;
    *count = 0;

    if (type->kind != TYPE_CLASS) return NULL;

    // Get fields from base class first
    FieldInfo** fields = NULL;
    int fieldCount = 0;
    
    if (type->classType.baseClass) {
        fields = get_fields(type->classType.baseClass, &fieldCount);
    }

    // Add fields from this class
    // In a real implementation, we would:
    // 1. Parse the class definition
    // 2. Extract field declarations
    // 3. Create FieldInfo for each field
    
    // For now, returning empty array
    *count = fieldCount;
    return fields;
}

MethodInfo** get_methods(Type* type, int* count) {
    if (!type || !count) return NULL;
    *count = 0;

    if (type->kind != TYPE_CLASS) return NULL;

    // Get methods from base class first
    MethodInfo** methods = NULL;
    int methodCount = 0;
    
    if (type->classType.baseClass) {
        methods = get_methods(type->classType.baseClass, &methodCount);
    }

    // Add methods from this class
    // In a real implementation, we would:
    // 1. Parse the class definition
    // 2. Extract method declarations
    // 3. Create MethodInfo for each method
    
    // For now, returning empty array
    *count = methodCount;
    return methods;
}

bool is_instance_of(AstNode* expr, Type* type) {
    if (!expr || !type) return false;
    
    Type* exprType = expr->inferredType;
    if (!exprType) return false;

    return are_types_compatible(exprType, type);
}

void print_type_info(TypeInfo* info) {
    if (!info) return;

    printf("Type: %s\n", info->name);
    if (info->baseType) {
        printf("Base type: %s\n", typeToString(info->baseType));
    }

    printf("Fields (%d):\n", info->fieldCount);
    for (int i = 0; i < info->fieldCount; i++) {
        printf("  %s: %s\n", 
               info->fields[i]->name,
               typeToString(info->fields[i]->type));
    }

    printf("Methods (%d):\n", info->methodCount);
    for (int i = 0; i < info->methodCount; i++) {
        printf("  %s(", info->methods[i]->name);
        for (int j = 0; j < info->methods[i]->paramCount; j++) {
            if (j > 0) printf(", ");
            printf("%s", typeToString(info->methods[i]->paramTypes[j]));
        }
        printf(") -> %s\n", typeToString(info->methods[i]->returnType));
    }
}

// Utility to get type information by name
TypeInfo* get_type_info_by_name(const char* name) {
    // Search in cache first
    for (int i = 0; i < typeCacheCount; i++) {
        if (strcmp(typeCache[i]->name, name) == 0) {
            return typeCache[i];
        }
    }
    return NULL;
}
