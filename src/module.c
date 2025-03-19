#include "module.h"
#include "parser.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declaración externa de lexerInit
extern void lexerInit(const char* source);

#define MAX_MODULES 256
static Module* loadedModules[MAX_MODULES];
static int moduleCount = 0;
static int debug_level = 1;  // Nivel de depuración por defecto

// Configuración de rutas de búsqueda
#define MAX_SEARCH_PATHS 16
static char* searchPaths[MAX_SEARCH_PATHS];
static int searchPathCount = 0;

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
    
    // Inicializar rutas de búsqueda con la ruta actual
    searchPathCount = 1;
    searchPaths[0] = strdup("./");
    
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
            if (loadedModules[i]->exports) {
                for (int j = 0; j < loadedModules[i]->exportCount; j++) {
                    freeAstNode(loadedModules[i]->exports[j].node);
                }
                free(loadedModules[i]->exports);
            }
            
            // Liberar imports
            free(loadedModules[i]->imports);
            
            // Liberar dependencies
            if (loadedModules[i]->dependencies) {
                for (int j = 0; j < loadedModules[i]->dependencyCount; j++) {
                    free(loadedModules[i]->dependencies[j]);
                }
                free(loadedModules[i]->dependencies);
            }
            
            // Liberar AST
            freeAst(loadedModules[i]->ast);
            
            free(loadedModules[i]);
            freed++;
        }
    }
    
    // Liberar rutas de búsqueda
    for (int i = 0; i < searchPathCount; i++) {
        free(searchPaths[i]);
    }
    searchPathCount = 0;
    
    moduleCount = 0;
    logger_log(LOG_INFO, "Module system cleanup complete: %d modules freed", freed);
}

void module_set_search_paths(const char* paths[], int count) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_search_paths);
    
    // Liberar rutas existentes
    for (int i = 0; i < searchPathCount; i++) {
        free(searchPaths[i]);
    }
    
    // Establecer nuevas rutas
    searchPathCount = (count > MAX_SEARCH_PATHS) ? MAX_SEARCH_PATHS : count;
    for (int i = 0; i < searchPathCount; i++) {
        searchPaths[i] = strdup(paths[i]);
    }
    
    logger_log(LOG_INFO, "Module search paths updated: %d paths", searchPathCount);
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

Module* module_get_by_name(const char* name) {
    return find_loaded_module(name);
}

bool module_detect_circular_dependency(Module* module, const char* dependencyName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_detect_circular_dependency);
    
    if (!module || !dependencyName) {
        return false;
    }
    
    // Caso base: el módulo ya se está cargando (estamos en un ciclo)
    if (strcmp(module->name, dependencyName) == 0) {
        return true;
    }
    
    // Si el módulo está en proceso de carga, tenemos una dependencia circular
    if (module->isLoading) {
        logger_log(LOG_DEBUG, "Detected circular dependency: %s while loading %s", 
                  module->name, dependencyName);
        return true;
    }
    
    return false;
}

void module_add_dependency(Module* module, const char* dependencyName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_add_dependency);
    
    if (!module || !dependencyName) {
        return;
    }
    
    // Comprobar si ya existe la dependencia
    for (int i = 0; i < module->dependencyCount; i++) {
        if (strcmp(module->dependencies[i], dependencyName) == 0) {
            return; // Ya existe
        }
    }
    
    // Añadir la dependencia
    module->dependencyCount++;
    module->dependencies = realloc(module->dependencies, 
                                  module->dependencyCount * sizeof(char*));
    if (!module->dependencies) {
        logger_log(LOG_ERROR, "Failed to allocate memory for dependency in module %s", 
                  module->name);
        return;
    }
    
    module->dependencies[module->dependencyCount - 1] = strdup(dependencyName);
    
    logger_log(LOG_DEBUG, "Added dependency %s to module %s", 
              dependencyName, module->name);
}

Module* module_load(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_load);
    
    if (!name) {
        logger_log(LOG_ERROR, "Attempted to load module with NULL name");
        error_report("Module", __LINE__, 0, "NULL module name provided", ERROR_UNDEFINED);
        return NULL;
    }
    
    // Verificar si ya está cargado
    Module* existing = find_loaded_module(name);
    if (existing) {
        if (existing->isLoaded) {
            logger_log(LOG_DEBUG, "Module '%s' already loaded, reusing instance", name);
            return existing;
        } else if (existing->isLoading) {
            // Circular dependency detected
            logger_log(LOG_WARNING, "Circular dependency detected for module '%s'", name);
            error_report("Module", __LINE__, 0, "Circular dependency detected", ERROR_RUNTIME);
            return NULL;
        }
    }

    // Intentar encontrar el módulo en las rutas de búsqueda
    char path[1024];
    FILE* file = NULL;
    
    for (int i = 0; i < searchPathCount && !file; i++) {
        snprintf(path, sizeof(path), "%s%s.lyn", searchPaths[i], name);
        file = fopen(path, "r");
        
        if (file) {
            logger_log(LOG_INFO, "Found module '%s' at '%s'", name, path);
        }
    }
    
    if (!file) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Could not find module '%s' in search paths", name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_IO);
        return NULL;
    }
    
    // Crear nuevo módulo
    Module* module = calloc(1, sizeof(Module));
    if (!module) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for module '%s'", name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        fclose(file);
        return NULL;
    }
    
    strncpy(module->name, name, sizeof(module->name) - 1);
    strncpy(module->path, path, sizeof(module->path) - 1);
    module->isLoading = true;
    
    // Registrar el módulo en la tabla global, incluso antes de terminar de cargarlo
    // Esto permite detectar dependencias circulares
    if (moduleCount < MAX_MODULES) {
        loadedModules[moduleCount++] = module;
    }

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
        // No liberamos el módulo, ya que está en loadedModules
        // Marcamos como no cargando para futuras detecciones de ciclos
        module->isLoading = false;
        return NULL;
    }

    char* source = malloc(size + 1);
    if (!source) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory (%ld bytes) for module content", size);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        fclose(file);
        module->isLoading = false;
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
        module->isLoading = false;
        return NULL;
    }
    
    source[size] = '\0';
    fclose(file);

    // Parsear el módulo
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
        module->isLoading = false;
        return NULL;
    }

    // Extraer exports y asignarles nombres
    if (module->ast->type == AST_PROGRAM) {
        for (int i = 0; i < module->ast->program.statementCount; i++) {
            AstNode* node = module->ast->program.statements[i];
            const char* symbolName = NULL;
            bool isPublic = true; // Por defecto public
            
            // Determinar el nombre basado en el tipo de nodo
            switch (node->type) {
                case AST_FUNC_DEF:
                    symbolName = node->funcDef.name;
                    break;
                case AST_CLASS_DEF:
                    symbolName = node->classDef.name;
                    break;
                case AST_VAR_DECL:
                    symbolName = node->varDecl.name;
                    // Variables por defecto son privadas a menos que estén marcadas como export
                    isPublic = false;
                    break;
                case AST_IMPORT:
                    // Procesar imports
                    module_import(module, node->importStmt.moduleName);
                    continue;
                default:
                    continue;
            }
            
            if (symbolName) {
                module_add_export(module, symbolName, node, isPublic);
            }
        }
    }

    // Módulo cargado completamente
    module->isLoaded = true;
    module->isLoading = false;
    logger_log(LOG_INFO, "Module '%s' loaded successfully with %d exports", 
              name, module->exportCount);
    
    return module;
}

bool module_import(Module* target, const char* moduleName) {
    return module_import_with_alias(target, moduleName, "", false);
}

bool module_import_with_alias(Module* target, const char* moduleName, const char* alias, bool isQualified) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_import_with_alias);
    
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

    // Verificar dependencia circular
    if (module_detect_circular_dependency(target, moduleName)) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Circular dependency detected: %s imports %s", 
                target->name, moduleName);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_RUNTIME);
        return false;
    }

    // Cargar el módulo si no está cargado
    Module* imported = module_load(moduleName);
    if (!imported) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to load module '%s' for import", moduleName);
        logger_log(LOG_ERROR, "%s", errMsg);
        return false;
    }

    // Verificar importación duplicada
    for (int i = 0; i < target->importCount; i++) {
        if (target->imports[i].module == imported &&
            strcmp(target->imports[i].alias, alias) == 0 && 
            target->imports[i].isQualified == isQualified) {
            logger_log(LOG_WARNING, "Module '%s' already imported in '%s', skipping duplicate", 
                     moduleName, target->name);
            return true;  // Ya está importado, no es un error
        }
    }

    // Registrar dependencia
    module_add_dependency(target, moduleName);

    // Agregar a la lista de imports
    target->importCount++;
    target->imports = realloc(target->imports, target->importCount * sizeof(ImportedModule));
    if (!target->imports) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for imports in module '%s'", 
                target->name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        return false;
    }
    
    ImportedModule* newImport = &target->imports[target->importCount - 1];
    strncpy(newImport->name, moduleName, sizeof(newImport->name) - 1);
    strncpy(newImport->alias, alias, sizeof(newImport->alias) - 1);
    newImport->isQualified = isQualified;
    newImport->module = imported;
    
    logger_log(LOG_INFO, "Module '%s'%s%s imported into '%s'%s", 
              moduleName,
              alias[0] ? " as " : "",
              alias[0] ? alias : "",
              target->name,
              isQualified ? " (qualified)" : "");
    return true;
}

ExportedSymbol* module_find_export(Module* module, const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_find_export);
    
    if (!module || !name) {
        return NULL;
    }
    
    for (int i = 0; i < module->exportCount; i++) {
        if (strcmp(module->exports[i].name, name) == 0 && module->exports[i].isPublic) {
            return &module->exports[i];
        }
    }
    
    return NULL;
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

    // 1. Buscar primero en los exports locales
    ExportedSymbol* localSymbol = module_find_export(module, name);
    if (localSymbol) {
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Symbol '%s' found in exports of module '%s'", 
                      name, module->name);
        }
        return localSymbol->node;
    }

    // 2. Buscar en módulos importados no calificados
    for (int i = 0; i < module->importCount; i++) {
        // Saltamos importaciones calificadas, ya que requieren explicit namespace
        if (module->imports[i].isQualified) continue;
        
        ExportedSymbol* importedSymbol = module_find_export(module->imports[i].module, name);
        if (importedSymbol) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Symbol '%s' found in imported module '%s'", 
                          name, module->imports[i].name);
            }
            return importedSymbol->node;
        }
    }
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Symbol '%s' not found in module '%s' or its imports", 
                  name, module->name);
    }

    return NULL;
}

AstNode* module_resolve_qualified_symbol(Module* module, const char* moduleName, const char* symbolName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_resolve_qualified_symbol);
    
    if (!module || !moduleName || !symbolName) {
        return NULL;
    }
    
    // Buscar el módulo importado por nombre o alias
    for (int i = 0; i < module->importCount; i++) {
        if (strcmp(module->imports[i].name, moduleName) == 0 || 
            (module->imports[i].alias[0] && strcmp(module->imports[i].alias, moduleName) == 0)) {
            
            // Encontrado el módulo, buscar el símbolo
            ExportedSymbol* symbol = module_find_export(module->imports[i].module, symbolName);
            if (symbol) {
                return symbol->node;
            }
            
            break;
        }
    }
    
    logger_log(LOG_WARNING, "Qualified symbol '%s.%s' not found in module '%s'", 
               moduleName, symbolName, module->name);
    return NULL;
}

void module_add_export(Module* module, const char* name, AstNode* node, bool isPublic) {
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
        if (strcmp(module->exports[i].name, name) == 0) {
            logger_log(LOG_WARNING, "Symbol '%s' already exported in module '%s', overwriting", 
                     name, module->name);
            module->exports[i].node = node;
            module->exports[i].isPublic = isPublic;
            return;
        }
    }

    // Añadir nuevo símbolo exportado
    module->exportCount++;
    module->exports = realloc(module->exports, module->exportCount * sizeof(ExportedSymbol));
    if (!module->exports) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to allocate memory for exports in module '%s'", 
                module->name);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_MEMORY);
        return;
    }
    
    ExportedSymbol* newSymbol = &module->exports[module->exportCount - 1];
    strncpy(newSymbol->name, name, sizeof(newSymbol->name) - 1);
    newSymbol->node = node;
    newSymbol->type = NULL;  // Se puede inferir/asignar después
    newSymbol->isPublic = isPublic;
    
    logger_log(LOG_DEBUG, "Symbol '%s' %sexported in module '%s'", 
              name, isPublic ? "" : "(private) ", module->name);
}

void module_print_info(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_print_info);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to print info of NULL module");
        return;
    }
    
    logger_log(LOG_INFO, "=== Module: %s ===", module->name);
    logger_log(LOG_INFO, "Path: %s", module->path);
    logger_log(LOG_INFO, "Status: %s", 
              module->isLoaded ? "Loaded" : (module->isLoading ? "Loading" : "Unloaded"));
    logger_log(LOG_INFO, "Exports: %d", module->exportCount);
    logger_log(LOG_INFO, "Imports: %d", module->importCount);
    logger_log(LOG_INFO, "Dependencies: %d", module->dependencyCount);

    if (debug_level >= 2) {
        // Log detailed export information
        logger_log(LOG_DEBUG, "Exports:");
        for (int i = 0; i < module->exportCount; i++) {
            const char* symbolType = "unknown";
            
            if (module->exports[i].node) {
                switch (module->exports[i].node->type) {
                    case AST_FUNC_DEF: symbolType = "function"; break;
                    case AST_CLASS_DEF: symbolType = "class"; break;
                    case AST_VAR_DECL: symbolType = "variable"; break;
                    default: symbolType = "unknown"; break;
                }
            }
            
            logger_log(LOG_DEBUG, "  - %s (%s)%s", 
                      module->exports[i].name, 
                      symbolType,
                      module->exports[i].isPublic ? "" : " [private]");
        }
        
        // Log detailed import information
        logger_log(LOG_DEBUG, "Imports:");
        for (int i = 0; i < module->importCount; i++) {
            logger_log(LOG_DEBUG, "  - %s%s%s%s", 
                      module->imports[i].name,
                      module->imports[i].alias[0] ? " as " : "",
                      module->imports[i].alias[0] ? module->imports[i].alias : "",
                      module->imports[i].isQualified ? " (qualified)" : "");
        }
        
        // Log dependencies
        logger_log(LOG_DEBUG, "Dependencies:");
        for (int i = 0; i < module->dependencyCount; i++) {
            logger_log(LOG_DEBUG, "  - %s", module->dependencies[i]);
        }
    }
    
    // For console output in addition to logs
    printf("=== Module: %s ===\n", module->name);
    printf("Path: %s\n", module->path);
    printf("Status: %s\n", 
          module->isLoaded ? "Loaded" : (module->isLoading ? "Loading" : "Unloaded"));
    printf("Exports: %d\n", module->exportCount);
    printf("Imports: %d\n", module->importCount);
    printf("Dependencies: %d\n", module->dependencyCount);
    
    printf("\nExports:\n");
    for (int i = 0; i < module->exportCount; i++) {
        const char* symbolType = "unknown";
        
        if (module->exports[i].node) {
            switch (module->exports[i].node->type) {
                case AST_FUNC_DEF: symbolType = "function"; break;
                case AST_CLASS_DEF: symbolType = "class"; break;
                case AST_VAR_DECL: symbolType = "variable"; break;
                default: symbolType = "unknown"; break;
            }
        }
        
        printf("  - %s (%s)%s\n", 
              module->exports[i].name, 
              symbolType,
              module->exports[i].isPublic ? "" : " [private]");
    }
    
    printf("\nImports:\n");
    for (int i = 0; i < module->importCount; i++) {
        printf("  - %s%s%s%s\n", 
              module->imports[i].name,
              module->imports[i].alias[0] ? " as " : "",
              module->imports[i].alias[0] ? module->imports[i].alias : "",
              module->imports[i].isQualified ? " (qualified)" : "");
    }
    
    printf("\nDependencies:\n");
    for (int i = 0; i < module->dependencyCount; i++) {
        printf("  - %s\n", module->dependencies[i]);
    }
    
    printf("==================\n");
}

// Funciones adicionales de diagnóstico

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
