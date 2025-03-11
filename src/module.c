#include "module.h"
#include "parser.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Agregar la declaración de lexerInit aquí
extern void lexerInit(const char* source);

#define MAX_MODULES 256
static Module* loadedModules[MAX_MODULES];
static int moduleCount = 0;

// ...existing code...

void module_system_init(void) {
    moduleCount = 0;
    logger_log(LOG_INFO, "Sistema de módulos inicializado");
}

void module_system_cleanup(void) {
    for (int i = 0; i < moduleCount; i++) {
        if (loadedModules[i]) {
            // Liberar exports
            for (int j = 0; j < loadedModules[i]->exportCount; j++) {
                freeAstNode(loadedModules[i]->exports[j]);
            }
            free(loadedModules[i]->exports);
            
            // Liberar imports
            free(loadedModules[i]->imports);
            
            // Liberar AST
            freeAst(loadedModules[i]->ast);
            
            free(loadedModules[i]);
        }
    }
    moduleCount = 0;
    logger_log(LOG_INFO, "Sistema de módulos liberado");
}

static Module* find_loaded_module(const char* name) {
    for (int i = 0; i < moduleCount; i++) {
        if (strcmp(loadedModules[i]->name, name) == 0) {
            return loadedModules[i];
        }
    }
    return NULL;
}

Module* module_load(const char* name) {
    // Verificar si ya está cargado
    Module* existing = find_loaded_module(name);
    if (existing) {
        logger_log(LOG_DEBUG, "Módulo '%s' ya cargado", name);
        return existing;
    }

    // Construir path del módulo
    char path[1024];
    snprintf(path, sizeof(path), "%s.lyn", name);

    // Abrir y leer el archivo
    FILE* file = fopen(path, "r");
    if (!file) {
        error_report("module", 0, 0, "No se pudo abrir el archivo del módulo");
        return NULL;
    }

    // Crear nuevo módulo
    Module* module = malloc(sizeof(Module));
    strncpy(module->name, name, sizeof(module->name) - 1);
    strncpy(module->path, path, sizeof(module->path) - 1);
    module->exports = NULL;
    module->exportCount = 0;
    module->imports = NULL;
    module->importCount = 0;
    module->ast = NULL;

    // Leer contenido del archivo
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = malloc(size + 1);
    fread(source, 1, size, file);
    source[size] = '\0';
    fclose(file);

    // Parsear el módulo
    lexerInit(source);
    module->ast = parseProgram();
    free(source);

    if (!module->ast) {
        error_report("module", 0, 0, "Error al parsear el módulo");
        free(module);
        return NULL;
    }

    // Registrar el módulo
    if (moduleCount < MAX_MODULES) {
        loadedModules[moduleCount++] = module;
        logger_log(LOG_INFO, "Módulo '%s' cargado exitosamente", name);
    } else {
        error_report("module", 0, 0, "Número máximo de módulos excedido");
        free(module);
        return NULL;
    }

    return module;
}

bool module_import(Module* target, const char* moduleName) {
    if (!target) return false;

    // Cargar el módulo si no está cargado
    Module* imported = module_load(moduleName);
    if (!imported) return false;

    // Agregar a la lista de imports
    target->importCount++;
    target->imports = realloc(target->imports, target->importCount * sizeof(Module*));
    
    // Usar un cast para asegurar que los tipos sean compatibles
    target->imports[target->importCount - 1] = (struct Module*)imported;

    logger_log(LOG_INFO, "Módulo '%s' importado en '%s'", moduleName, target->name);
    return true;
}

AstNode* module_resolve_symbol(Module* module, const char* name) {
    if (!module) return NULL;

    // Buscar en exports
    for (int i = 0; i < module->exportCount; i++) {
        // Comparar nombre del símbolo según su tipo
        const char* symbolName = NULL;
        switch (module->exports[i]->type) {
            case AST_FUNC_DEF:
                symbolName = module->exports[i]->funcDef.name;
                break;
            case AST_CLASS_DEF:
                symbolName = module->exports[i]->classDef.name;
                break;
            case AST_VAR_DECL:
                symbolName = module->exports[i]->varDecl.name;
                break;
            default:
                continue;
        }
        if (strcmp(symbolName, name) == 0) {
            return module->exports[i];
        }
    }

    // Buscar en módulos importados
    for (int i = 0; i < module->importCount; i++) {
        // Usar un cast para resolver el problema de tipos incompatibles
        AstNode* symbol = module_resolve_symbol((Module*)module->imports[i], name);
        if (symbol) return symbol;
    }

    return NULL;
}

void module_add_export(Module* module, const char* name, AstNode* node) {
    if (!module || !name || !node) return;

    module->exportCount++;
    module->exports = realloc(module->exports, module->exportCount * sizeof(AstNode*));
    module->exports[module->exportCount - 1] = node;
    
    logger_log(LOG_DEBUG, "Símbolo '%s' exportado en módulo '%s'", name, module->name);
}

void module_print_info(Module* module) {
    if (!module) return;

    printf("=== Módulo: %s ===\n", module->name);
    printf("Path: %s\n", module->path);
    printf("Exports: %d\n", module->exportCount);
    printf("Imports: %d\n", module->importCount);
    
    printf("\nExports:\n");
    for (int i = 0; i < module->exportCount; i++) {
        const char* symbolName = "unknown";
        switch (module->exports[i]->type) {
            case AST_FUNC_DEF:
                symbolName = module->exports[i]->funcDef.name;
                break;
            case AST_CLASS_DEF:
                symbolName = module->exports[i]->classDef.name;
                break;
            case AST_VAR_DECL:
                symbolName = module->exports[i]->varDecl.name;
                break;
        }
        printf("  - %s\n", symbolName);
    }
    
    printf("\nImports:\n");
    for (int i = 0; i < module->importCount; i++) {
        // Usar un cast para acceder a los campos del módulo importado
        printf("  - %s\n", ((Module*)module->imports[i])->name);
    }
    printf("==================\n");
}