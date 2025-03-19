#include "module.h"
#include "parser.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declaraci?n externa de lexerInit
extern void lexerInit(const char* source);

#define MAX_MODULES 256
static Module* loadedModules[MAX_MODULES];
static int moduleCount = 0;
static int debug_level = 1;  // Nivel de depuraci?n por defecto

void module_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Module system debug level set to %d", level);
}

int module_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_get_debug_level);
    return debug_level;
}

void module_system_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_system_init);
    
    moduleCount = 0;
    logger_log(LOG_INFO, "Module system initialized");
}

void module_system_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_system_cleanup);
    
    int freed = 0;
    for (int i = 0; i < moduleCount; i++) {
        if (loadedModules[i]) {
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Cleaning up module '%s'", loadedModules[i]->name);
            }
            
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
            freed++;
        }
    }
    moduleCount = 0;
    logger_log(LOG_INFO, "Module system cleanup complete: %d modules freed", freed);
}

static Module* find_loaded_module(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)find_loaded_module);
    
    if (!name) {
        logger_log(LOG_WARNING, "Attempted to find module with NULL name");
        return NULL;
    }
    
    for (int i = 0; i < moduleCount; i++) {
        if (strcmp(loadedModules[i]->name, name) == 0) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Found loaded module '%s' at index %d", name, i);
            }
            return loadedModules[i];
        }
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Module '%s' not found in loaded modules", name);
    }
    return NULL;
}

Module* module_load(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_load);
    
    if (!name) {
        logger_log(LOG_ERROR, "Attempted to load module with NULL name");
        error_report("Module", __LINE__, 0, "NULL module name provided", ERROR_UNDEFINED);
        return NULL;
    }
    
    // Verificar si ya est? cargado
    Module* existing = find_loaded_module(name);
    if (existing) {
        logger_log(LOG_DEBUG, "Module '%s' already loaded, reusing instance", name);
        return existing;
    }

    // Construir path del m?dulo
    char path[1024];
    snprintf(path, sizeof(path), "%s.lyn", name);
    logger_log(LOG_INFO, "Loading module '%s' from '%s'", name, path);

    // Abrir y leer el archivo
    FILE* file = fopen(path, "r");
    if (!file) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Could not open module file '%s'", path);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_IO);
        return NULL;
    }
    
    // Crear nuevo m?dulo
    Module* module = malloc(sizeof(Module));
    if (!module) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for module '%s'", name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        fclose(file);
        return NULL;
    }
    memset(module, 0, sizeof(Module));  // Initialize all fields to zero/NULL
    strncpy(module->name, name, sizeof(module->name) - 1);
    strncpy(module->path, path, sizeof(module->path) - 1);

    // Leer contenido del archivo
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Empty or invalid module file '%s'", path);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_IO);
        fclose(file);
        free(module);
        return NULL;
    }

    char* source = malloc(size + 1);
    if (!source) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory (%ld bytes) for module content", size);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        fclose(file);
        free(module);
        return NULL;
    }
    
    size_t read = fread(source, 1, size, file);
    if (read != (size_t)size) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to read module file '%s' (read %zu of %ld bytes)", 
                path, read, size);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_IO);
        fclose(file);
        free(source);
        free(module);
        return NULL;
    }
    
    source[size] = '\0';
    fclose(file);

    // Parsear el m?dulo
    lexerInit(source);
    module->ast = parseProgram();
    
    // Make it available to the error system for context
    error_set_source(source);
    
    // Now source can be freed since the parser has built the AST
    free(source);

    if (!module->ast) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Error parsing module '%s'", name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_SYNTAX);
        free(module);
        return NULL;
    }

    // Registrar el m?dulo
    if (moduleCount < MAX_MODULES) {
        loadedModules[moduleCount++] = module;
        logger_log(LOG_INFO, "Module '%s' loaded successfully", name);
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Module registered at index %d, total modules: %d", 
                      moduleCount-1, moduleCount);
        }
    } else {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Maximum number of modules (%d) exceeded", MAX_MODULES);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_RUNTIME);
        free(module);
        return NULL;
    }

    return module;
}

bool module_import(Module* target, const char* moduleName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_import);
    
    if (!target) {
        logger_log(LOG_ERROR, "Attempted to import into NULL target module");
        error_report("Module", __LINE__, 0, "NULL target module", ERROR_UNDEFINED);
        return false;
    }
    
    if (!moduleName) {
        logger_log(LOG_ERROR, "Attempted to import NULL module name into '%s'", target->name);
        error_report("Module", __LINE__, 0, "NULL module name in import", ERROR_UNDEFINED);
        return false;
    }

    // Cargar el m?dulo si no est? cargado
    Module* imported = module_load(moduleName);
    if (!imported) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to load module '%s' for import", moduleName);
        logger_log(LOG_ERROR, "%s", errMsg);
        return false;
    }

    // Verificar dependencia circular
    for (int i = 0; i < target->importCount; i++) {
        if (target->imports[i] == (struct Module*)imported) {
            logger_log(LOG_WARNING, "Module '%s' already imported in '%s', skipping duplicate", 
                     moduleName, target->name);
            return true;  // Ya est? importado, no es un error
        }
    }

    // Agregar a la lista de imports
    target->importCount++;
    target->imports = realloc(target->imports, target->importCount * sizeof(Module*));
    if (!target->imports) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for imports in module '%s'", 
                target->name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        return false;
    }
    
    // Se agrega el cast para corregir el error de tipos
    target->imports[target->importCount - 1] = (struct Module*) imported;
    
    logger_log(LOG_INFO, "Module '%s' imported into '%s'", moduleName, target->name);
    return true;
}

AstNode* module_resolve_symbol(Module* module, const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_resolve_symbol);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to resolve symbol in NULL module");
        return NULL;
    }
    
    if (!name) {
        logger_log(LOG_WARNING, "Attempted to resolve NULL symbol name in module '%s'", module->name);
        return NULL;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Resolving symbol '%s' in module '%s'", name, module->name);
    }

    // Buscar en exports
    for (int i = 0; i < module->exportCount; i++) {
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
        if (symbolName && strcmp(symbolName, name) == 0) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Symbol '%s' found in exports of module '%s'", 
                          name, module->name);
            }
            return module->exports[i];
        }
    }

    // Buscar en m?dulos importados
    for (int i = 0; i < module->importCount; i++) {
        AstNode* symbol = module_resolve_symbol((Module*)module->imports[i], name);
        if (symbol) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Symbol '%s' found in imported module '%s'", 
                          name, ((Module*)module->imports[i])->name);
            }
            return symbol;
        }
    }
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Symbol '%s' not found in module '%s' or its imports", 
                  name, module->name);
    }

    return NULL;
}

void module_add_export(Module* module, const char* name, AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_add_export);
    
    if (!module) {
        logger_log(LOG_ERROR, "Attempted to add export to NULL module");
        error_report("Module", __LINE__, 0, "NULL module in add_export", ERROR_UNDEFINED);
        return;
    }
    
    if (!name) {
        logger_log(LOG_ERROR, "Attempted to add NULL export name to module '%s'", module->name);
        error_report("Module", __LINE__, 0, "NULL export name", ERROR_UNDEFINED);
        return;
    }
    
    if (!node) {
        logger_log(LOG_ERROR, "Attempted to add NULL export node to module '%s'", module->name);
        error_report("Module", __LINE__, 0, "NULL export node", ERROR_UNDEFINED);
        return;
    }

    // Verificar duplicado
    for (int i = 0; i < module->exportCount; i++) {
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
        }
        if (symbolName && strcmp(symbolName, name) == 0) {
            logger_log(LOG_WARNING, "Symbol '%s' already exported in module '%s', overwriting", 
                     name, module->name);
            module->exports[i] = node;
            return;
        }
    }

    module->exportCount++;
    module->exports = realloc(module->exports, module->exportCount * sizeof(AstNode*));
    if (!module->exports) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for exports in module '%s'", 
                module->name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        return;
    }
    
    module->exports[module->exportCount - 1] = node;
    logger_log(LOG_DEBUG, "Symbol '%s' exported in module '%s'", name, module->name);
}

void module_print_info(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_print_info);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to print info of NULL module");
        return;
    }
    
    logger_log(LOG_INFO, "=== Module: %s ===", module->name);
    logger_log(LOG_INFO, "Path: %s", module->path);
    logger_log(LOG_INFO, "Exports: %d", module->exportCount);
    logger_log(LOG_INFO, "Imports: %d", module->importCount);

    if (debug_level >= 2) {
        // Log detailed export information
        logger_log(LOG_DEBUG, "Exports:");
        for (int i = 0; i < module->exportCount; i++) {
            const char* symbolName = "unknown";
            const char* symbolType = "unknown";
            
            switch (module->exports[i]->type) {
                case AST_FUNC_DEF:
                    symbolName = module->exports[i]->funcDef.name;
                    symbolType = "function";
                    break;
                case AST_CLASS_DEF:
                    symbolName = module->exports[i]->classDef.name;
                    symbolType = "class";
                    break;
                case AST_VAR_DECL:
                    symbolName = module->exports[i]->varDecl.name;
                    symbolType = "variable";
                    break;
            }
            logger_log(LOG_DEBUG, "  - %s (%s)", symbolName, symbolType);
        }
        
        // Log detailed import information
        logger_log(LOG_DEBUG, "Imports:");
        for (int i = 0; i < module->importCount; i++) {
            logger_log(LOG_DEBUG, "  - %s", ((Module*)module->imports[i])->name);
        }
    }
    
    // For console output in addition to logs
    printf("=== Module: %s ===\n", module->name);
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
        printf("  - %s\n", ((Module*)module->imports[i])->name);
    }
    printf("==================\n");
}

// Funciones adicionales de diagn?stico

int module_count_loaded(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_count_loaded);
    return moduleCount;
}

const char* module_get_name(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_get_name);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to get name of NULL module");
        return "NULL";
    }
    
    return module->name;
}
