#ifndef LYN_MODULE_H
#define LYN_MODULE_H

#include "ast.h"
#include <stdbool.h>

typedef struct {
    char name[256];         // Nombre del módulo
    char path[1024];        // Ruta del archivo
    AstNode** exports;      // Símbolos exportados
    int exportCount;
    struct Module** imports; // Módulos importados
    int importCount;
    AstNode* ast;           // AST del módulo
} Module;

// Inicialización y limpieza
void module_system_init(void);
void module_system_cleanup(void);

// Funciones principales
Module* module_load(const char* name);
bool module_import(Module* target, const char* moduleName);
AstNode* module_resolve_symbol(Module* module, const char* name);

// Funciones de utilidad
void module_add_export(Module* module, const char* name, AstNode* node);
void module_print_info(Module* module);

#endif // LYN_MODULE_H