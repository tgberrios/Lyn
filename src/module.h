#ifndef LYN_MODULE_H
#define LYN_MODULE_H

#include "ast.h"
#include "error.h"
#include "logger.h"
#include "types.h"  // Añadido para tener acceso a la estructura Type
#include <stdbool.h>

// Estructura para representar un símbolo exportado
typedef struct ExportedSymbol {
    char name[256];         // Nombre del símbolo exportado
    AstNode* node;          // Nodo AST correspondiente
    Type* type;             // Tipo del símbolo (opcional)
    bool isPublic;          // Si el símbolo es público o interno
} ExportedSymbol;

// Estructura para representar un módulo importado
typedef struct ImportedModule {
    char name[256];         // Nombre del módulo importado
    char alias[256];        // Alias utilizado (puede ser vacío)
    bool isQualified;       // Si la importación es calificada (requires namespace)
    struct Module* module;  // Puntero al módulo importado
} ImportedModule;

typedef struct Module {
    char name[256];         // Nombre del módulo
    char path[1024];        // Ruta del archivo
    
    // Sistema de namespace y exports
    ExportedSymbol* exports;// Símbolos exportados
    int exportCount;
    
    // Sistema de imports
    ImportedModule* imports; // Módulos importados
    int importCount;
    
    // Dependencias
    char** dependencies;    // Lista de nombres de módulos dependientes
    int dependencyCount;
    
    bool isLoaded;          // Si el módulo ha sido cargado completamente
    bool isLoading;         // Para detectar dependencias circulares
    
    AstNode* ast;           // AST del módulo
} Module;

// Inicialización y limpieza
void module_system_init(void);
void module_system_cleanup(void);

// Nivel de depuración para el sistema de módulos (0=mínimo, 3=máximo)
void module_set_debug_level(int level);
int module_get_debug_level(void);

// Funciones principales
Module* module_load(const char* name);
bool module_import(Module* target, const char* moduleName);
bool module_import_with_alias(Module* target, const char* moduleName, const char* alias, bool isQualified);
AstNode* module_resolve_symbol(Module* module, const char* name);
AstNode* module_resolve_qualified_symbol(Module* module, const char* moduleName, const char* symbolName);

// Funciones de utilidad
void module_add_export(Module* module, const char* name, AstNode* node, bool isPublic);
void module_print_info(Module* module);
bool module_detect_circular_dependency(Module* module, const char* dependencyName);
void module_add_dependency(Module* module, const char* dependencyName);
ExportedSymbol* module_find_export(Module* module, const char* name);

// Funciones de diagnóstico
int module_count_loaded(void);
const char* module_get_name(Module* module);
Module* module_get_by_name(const char* name);
void module_set_search_paths(const char* paths[], int count);

#endif // LYN_MODULE_H