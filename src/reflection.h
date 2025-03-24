#ifndef REFLECTION_H
#define REFLECTION_H

#include "types.h"
#include "ast.h"

// Runtime type information and reflection capabilities
typedef struct RuntimeType {
    TypeInfo* typeInfo;
    void* vtable;        // Virtual method table
    void* metadata;      // Additional type metadata
} RuntimeType;

// Reflection API
RuntimeType* get_runtime_type(AstNode* expr);
bool is_instance_of(AstNode* expr, Type* type);
AstNode* create_instance(Type* type);
bool set_field(AstNode* obj, const char* fieldName, AstNode* value);
AstNode* get_field(AstNode* obj, const char* fieldName);
AstNode* invoke_method(AstNode* obj, const char* methodName, AstNode** args, int argCount);
bool has_method(AstNode* obj, const char* methodName);
bool has_field(AstNode* obj, const char* fieldName);

// Type comparison and checking
bool is_subtype(Type* type1, Type* type2);
bool is_interface(Type* type);
bool implements_interface(Type* type, Type* interface);

// Reflection utilities
const char* type_to_string(Type* type);
void print_type_info(TypeInfo* info);
TypeInfo* get_type_info_by_name(const char* name);

#endif
