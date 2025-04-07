/**
 * @file module.c
 * @brief Module system implementation for the Lyn compiler
 * 
 * This file implements a comprehensive module system that handles:
 * - Module loading and unloading
 * - Module dependencies and circular dependency detection
 * - Symbol resolution and export/import management
 * - Module search paths and file handling
 */

#include "module.h"
#include "parser.h"
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Declaraciones para soporte de carga dinámica
#ifdef _WIN32
    #include <windows.h>
    #define DYNLIB_HANDLE HMODULE
    #define DYNLIB_LOAD(a) LoadLibrary(a)
    #define DYNLIB_GETSYM(a, b) GetProcAddress(a, b)
    #define DYNLIB_UNLOAD(a) FreeLibrary(a)
#else
    #include <dlfcn.h>
    #define DYNLIB_HANDLE void*
    #define DYNLIB_LOAD(a) dlopen(a, RTLD_LAZY)
    #define DYNLIB_GETSYM(a, b) dlsym(a, b)
    #define DYNLIB_UNLOAD(a) dlclose(a)
#endif

// Forward declarations
static Module* module_load_impl(const char* name, Module* module);

// External declaration of lexerInit
extern void lexerInit(const char* source);

#define MAX_MODULES 256  ///< Maximum number of modules that can be loaded simultaneously
static Module* loadedModules[MAX_MODULES];  ///< Array of loaded modules
static int moduleCount = 0;  ///< Current number of loaded modules
static int debug_level = 1;  ///< Default debug level for the module system

// Search path configuration
#define MAX_SEARCH_PATHS 16  ///< Maximum number of module search paths
static char* searchPaths[MAX_SEARCH_PATHS];  ///< Array of module search paths
static int searchPathCount = 0;  ///< Current number of search paths

/**
 * @brief Sets the debug level for the module system
 * 
 * @param level The new debug level (0-3)
 */
void module_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Module system debug level set to %d", level);
}

/**
 * @brief Gets the current debug level for the module system
 * 
 * @return int Current debug level (0-3)
 */
int module_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_get_debug_level);
    return debug_level;
}

/**
 * @brief Initializes the module system
 * 
 * Sets up the module system with default search paths and initializes internal state.
 */
void module_system_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_system_init);
    
    moduleCount = 0;
    
    // Initialize search paths with current directory
    searchPathCount = 1;
    searchPaths[0] = strdup("./");
    
    logger_log(LOG_INFO, "Module system initialized");
}

/**
 * @brief Cleans up the module system
 * 
 * Frees all loaded modules, their resources, and search paths.
 */
void module_system_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_system_cleanup);
    
    int freed = 0;
    for (int i = 0; i < moduleCount; i++) {
        if (loadedModules[i]) {
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Cleaning up module '%s'", loadedModules[i]->name);
            }
            
            // Free exports
            if (loadedModules[i]->exports) {
                for (int j = 0; j < loadedModules[i]->exportCount; j++) {
                    freeAstNode(loadedModules[i]->exports[j].node);
                }
                free(loadedModules[i]->exports);
            }
            
            // Free imports
            free(loadedModules[i]->imports);
            
            // Free dependencies
            if (loadedModules[i]->dependencies) {
                for (int j = 0; j < loadedModules[i]->dependencyCount; j++) {
                    free(loadedModules[i]->dependencies[j]);
                }
                free(loadedModules[i]->dependencies);
            }
            
            // Free AST
            freeAst(loadedModules[i]->ast);
            
            free(loadedModules[i]);
            freed++;
        }
    }
    
    // Free search paths
    for (int i = 0; i < searchPathCount; i++) {
        free(searchPaths[i]);
    }
    searchPathCount = 0;
    
    moduleCount = 0;
    logger_log(LOG_INFO, "Module system cleanup complete: %d modules freed", freed);
}

/**
 * @brief Sets the module search paths
 * 
 * Updates the list of directories where the module system looks for module files.
 * 
 * @param paths Array of search path strings
 * @param count Number of paths in the array
 */
void module_set_search_paths(const char* paths[], int count) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_search_paths);
    
    // Free existing paths
    for (int i = 0; i < searchPathCount; i++) {
        free(searchPaths[i]);
    }
    
    // Set new paths
    searchPathCount = (count > MAX_SEARCH_PATHS) ? MAX_SEARCH_PATHS : count;
    for (int i = 0; i < searchPathCount; i++) {
        searchPaths[i] = strdup(paths[i]);
    }
    
    logger_log(LOG_INFO, "Module search paths updated: %d paths", searchPathCount);
}

/**
 * @brief Finds a module by name in the loaded modules list
 * 
 * @param name Name of the module to find
 * @return Module* Pointer to the found module, or NULL if not found
 */
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

/**
 * @brief Gets a module by name
 * 
 * @param name Name of the module to get
 * @return Module* Pointer to the module, or NULL if not found
 */
Module* module_get_by_name(const char* name) {
    return find_loaded_module(name);
}

/**
 * @brief Detects circular dependencies between modules
 * 
 * @param module The module being checked
 * @param dependencyName Name of the potential dependency
 * @return bool true if a circular dependency is detected
 */
bool module_detect_circular_dependency(Module* module, const char* dependencyName) {
    if (!module || !dependencyName || !*dependencyName) {
        return false;
    }
    
    logger_log(LOG_DEBUG, "Checking for circular dependency: %s -> %s", 
               module->name, dependencyName);
    
    // Si el módulo actual está en proceso de carga, comprobar si estamos intentando
    // importar el mismo módulo recursivamente
    if (module->isLoading && strcmp(module->name, dependencyName) == 0) {
        logger_log(LOG_WARNING, "Direct circular dependency detected: %s depends on itself", 
                   module->name);
        return true;
    }
    
    // Verificar toda la cadena de dependencias
    for (int i = 0; i < module->dependencyCount; i++) {
        // Comprobamos si alguna dependencia directa coincide con el nombre buscado
        if (strcmp(module->dependencies[i], dependencyName) == 0) {
            logger_log(LOG_WARNING, "Circular dependency detected: %s -> %s", 
                       module->name, dependencyName);
            return true;
        }
        
        // Comprobamos recursivamente las dependencias de cada dependencia
        Module* depModule = module_get_by_name(module->dependencies[i]);
        if (depModule) {
            if (module_detect_circular_dependency(depModule, dependencyName)) {
                logger_log(LOG_WARNING, "Indirect circular dependency detected: %s -> %s through %s", 
                           module->name, dependencyName, module->dependencies[i]);
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Adds a dependency to a module
 * 
 * @param module The module to add the dependency to
 * @param dependencyName Name of the dependency to add
 */
void module_add_dependency(Module* module, const char* dependencyName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_add_dependency);
    
    if (!module || !dependencyName) {
        return;
    }
    
    // Check if dependency already exists
    for (int i = 0; i < module->dependencyCount; i++) {
        if (strcmp(module->dependencies[i], dependencyName) == 0) {
            return; // Already exists
        }
    }
    
    // Add the dependency
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

/**
 * @brief Loads a module with caching
 * 
 * @param name Name of the module to load
 * @param forceFresh Whether to force a fresh load ignoring cache
 * @return Module* Pointer to the loaded module, or NULL if loading failed
 */
Module* module_load_cached(const char* name, bool forceFresh) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_load_cached);
    
    if (!name) {
        logger_log(LOG_ERROR, "Attempted to load module with NULL name");
        error_report("Module", __LINE__, 0, "NULL module name provided", ERROR_UNDEFINED);
        return NULL;
    }
    
    // Check if already loaded
    Module* existing = find_loaded_module(name);
    if (existing) {
        if (existing->isLoaded) {
            // Check if we should use the cached version
            if (!forceFresh && existing->isCached) {
                // Check if the file has been modified since last load
                char path[1024];
                FILE* file = NULL;
                
                for (int i = 0; i < searchPathCount && !file; i++) {
                    snprintf(path, sizeof(path), "%s%s.lyn", searchPaths[i], name);
                    file = fopen(path, "r");
                }
                
                if (file) {
                    struct stat statBuf;
                    fstat(fileno(file), &statBuf);
                    time_t modificationTime = statBuf.st_mtime;
                    fclose(file);
                    
                    if (modificationTime <= existing->lastModified) {
                        logger_log(LOG_INFO, "Using cached version of module '%s'", name);
                        return existing;
                    } else {
                        logger_log(LOG_INFO, "Module '%s' has been modified, reloading", name);
                        // File has been modified, we need to reload
                    }
                } else {
                    // Cannot open the file, but we have a cached version
                    logger_log(LOG_WARNING, "Cannot open module file '%s.lyn', using cached version", name);
                    return existing;
                }
            } else if (forceFresh) {
                logger_log(LOG_INFO, "Forced reload of module '%s'", name);
                // Need to unload the existing module first
                // For now, we'll just reuse the existing slot
                
                // Free exports
                if (existing->exports) {
                    for (int j = 0; j < existing->exportCount; j++) {
                        freeAstNode(existing->exports[j].node);
                    }
                    free(existing->exports);
                    existing->exports = NULL;
                    existing->exportCount = 0;
                }
                
                // Free imports
                if (existing->imports) {
                    for (int j = 0; j < existing->importCount; j++) {
                        if (existing->imports[j].symbols) {
                            free(existing->imports[j].symbols);
                        }
                    }
                    free(existing->imports);
                    existing->imports = NULL;
                    existing->importCount = 0;
                }
                
                // Free dependencies
                if (existing->dependencies) {
                    for (int j = 0; j < existing->dependencyCount; j++) {
                        free(existing->dependencies[j]);
                    }
                    free(existing->dependencies);
                    existing->dependencies = NULL;
                    existing->dependencyCount = 0;
                }
                
                // Free AST
                freeAst(existing->ast);
                existing->ast = NULL;
                
                // Reset flags
                existing->isLoaded = false;
                existing->isLoading = true;
                existing->isCached = false;
                
                // Reuse the existing module struct
                return module_load_impl(name, existing);
            } else {
                logger_log(LOG_DEBUG, "Module '%s' already loaded, reusing instance", name);
                return existing;
            }
        } else if (existing->isLoading) {
            // Circular dependency detected
            logger_log(LOG_WARNING, "Circular dependency detected for module '%s'", name);
            error_report("Module", __LINE__, 0, "Circular dependency detected", ERROR_RUNTIME);
            return NULL;
        }
    }
    
    // Not loaded or needs reload, do a normal load
    return module_load(name);
}

// Helper function for implementing module loading logic
static Module* module_load_impl(const char* name, Module* module) {
    // Try to find the module in search paths
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
    
    // Get file modification time for cache validation
    struct stat statBuf;
    fstat(fileno(file), &statBuf);
    time_t modificationTime = statBuf.st_mtime;
    
    // Create new module if not reusing
    if (!module) {
        module = calloc(1, sizeof(Module));
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
        
        // Initialize version
        module->version.major = 1;
        module->version.minor = 0;
        module->version.patch = 0;
        
        // Register module in global table, even before loading is complete
        // This allows circular dependency detection
        if (moduleCount < MAX_MODULES) {
            loadedModules[moduleCount++] = module;
        }
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Empty or invalid module file '%s'", path);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_IO);
        fclose(file);
        // Don't free module as it's in loadedModules
        // Mark as not loading for future cycle detection
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

    // Parse the module
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

    // Extract exports and assign names
    if (module->ast->type == AST_PROGRAM) {
        for (int i = 0; i < module->ast->program.statementCount; i++) {
            AstNode* node = module->ast->program.statements[i];
            const char* symbolName = NULL;
            ExportVisibility visibility = EXPORT_PRIVATE;
            
            // Determine name based on node type
            switch (node->type) {
                case AST_FUNC_DEF:
                    symbolName = node->funcDef.name;
                    visibility = EXPORT_PUBLIC; // Functions are public by default
                    break;
                case AST_CLASS_DEF:
                    symbolName = node->classDef.name;
                    visibility = EXPORT_PUBLIC; // Classes are public by default
                    break;
                case AST_VAR_DECL:
                    symbolName = node->varDecl.name;
                    // Variables are private by default unless marked as export
                    visibility = EXPORT_PRIVATE;
                    break;
                case AST_IMPORT:
                    // Process imports without using non-existent fields like isQualified and alias
                    // Just import the module with the default settings
                    module_import(module, node->importStmt.moduleName);
                    continue;
                default:
                    continue;
            }
            
            if (symbolName) {
                module_add_export(module, symbolName, node, visibility);
            }
        }
    }

    // Update cache information
    module->lastModified = modificationTime;
    module->isCached = true;
    
    // Module fully loaded
    module->isLoaded = true;
    module->isLoading = false;
    logger_log(LOG_INFO, "Module '%s' loaded successfully with %d exports", 
              name, module->exportCount);
    
    return module;
}

/**
 * @brief Loads a module from disk
 * 
 * Searches for the module in the configured search paths, loads its content,
 * parses it, and processes its exports and imports.
 * 
 * @param name Name of the module to load
 * @return Module* Pointer to the loaded module, or NULL if loading failed
 */
Module* module_load(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_load);
    
    if (!name) {
        logger_log(LOG_ERROR, "Attempted to load module with NULL name");
        error_report("Module", __LINE__, 0, "NULL module name provided", ERROR_UNDEFINED);
        return NULL;
    }
    
    // Check if already loaded
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

    // Crear un módulo para intentar cargarlo dinámicamente
    Module* module = calloc(1, sizeof(Module));
    if (!module) {
        logger_log(LOG_ERROR, "Failed to allocate memory for module structure");
        error_report("Module", __LINE__, 0, "Memory allocation failed", ERROR_MEMORY);
        return NULL;
    }
    
    strncpy(module->name, name, sizeof(module->name) - 1);
    module->isLoading = true;
    
    // Register module in global table before loading
    if (moduleCount < MAX_MODULES) {
        loadedModules[moduleCount++] = module;
    }
    
    // Primero intentar cargar dinámicamente
    logger_log(LOG_DEBUG, "Attempting to load module '%s' dynamically", name);
    if (module_load_dynamic(module)) {
        // Éxito en la carga dinámica
        module->isLoaded = true;
        module->isLoading = false;
        logger_log(LOG_INFO, "Module '%s' loaded dynamically with %d exports", 
                  name, module->exportCount);
        return module;
    }
    
    // Si la carga dinámica falla, intentar el método tradicional
    logger_log(LOG_DEBUG, "Dynamic loading failed, attempting to load module '%s' from source", name);
    return module_load_impl(name, module);
}

/**
 * @brief Reloads a module if its source file has changed
 * 
 * @param module The module to check and potentially reload
 * @return bool true if the module was reloaded
 */
bool module_hot_reload(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_hot_reload);
    
    if (!module) {
        logger_log(LOG_ERROR, "Attempted to hot reload NULL module");
        error_report("Module", __LINE__, 0, "NULL module in hot_reload", ERROR_UNDEFINED);
        return false;
    }
    
    // Check if the file exists and get its modification time
    FILE* file = NULL;
    struct stat statBuf;
    
    file = fopen(module->path, "r");
    if (!file) {
        logger_log(LOG_WARNING, "Failed to open module file '%s' for hot reload", module->path);
        return false;
    }
    
    fstat(fileno(file), &statBuf);
    time_t modificationTime = statBuf.st_mtime;
    fclose(file);
    
    // Compare modification times
    if (modificationTime <= module->lastModified) {
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Module '%s' unchanged, no need to reload", module->name);
        }
        return false;
    }
    
    logger_log(LOG_INFO, "Module '%s' changed, reloading", module->name);
    
    // Reload the module
    Module* reloaded = module_load_cached(module->name, true);
    if (!reloaded) {
        logger_log(LOG_ERROR, "Failed to reload module '%s'", module->name);
        return false;
    }
    
    return true;
}

/**
 * @brief Imports a module with an optional alias
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import
 * @param alias Optional alias for the imported module
 * @param mode Import mode (all, selective, qualified)
 * @return bool true if import was successful
 */
bool module_import_with_options(Module* target, const char* moduleName, const char* alias, ImportMode mode) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_import_with_options);
    
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

    // Check for circular dependency
    if (module_detect_circular_dependency(target, moduleName)) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Circular dependency detected: %s imports %s", 
                target->name, moduleName);
        logger_log(LOG_ERROR, "%s", errMsg);
        error_report("Module", __LINE__, 0, errMsg, ERROR_RUNTIME);
        return false;
    }

    // Load the module if not already loaded, using caching if available
    Module* imported = module_load_cached(moduleName, false);
    if (!imported) {
        char errMsg[1024];
        snprintf(errMsg, sizeof(errMsg), "Failed to load module '%s' for import", moduleName);
        logger_log(LOG_ERROR, "%s", errMsg);
        return false;
    }

    // Check for duplicate import
    for (int i = 0; i < target->importCount; i++) {
        if (target->imports[i].module == imported &&
            strcmp(target->imports[i].alias, alias ? alias : "") == 0 && 
            target->imports[i].mode == mode) {
            logger_log(LOG_WARNING, "Module '%s' already imported in '%s', skipping duplicate", 
                     moduleName, target->name);
            return true;  // Already imported, not an error
        }
    }

    // Register dependency
    module_add_dependency(target, moduleName);

    // Add to imports list
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
    strncpy(newImport->alias, alias ? alias : "", sizeof(newImport->alias) - 1);
    newImport->mode = mode;
    newImport->module = imported;
    newImport->symbols = NULL;
    newImport->symbolCount = 0;
    
    logger_log(LOG_INFO, "Module '%s'%s%s imported into '%s' (mode: %d)", 
              moduleName,
              alias && alias[0] ? " as " : "",
              alias && alias[0] ? alias : "",
              target->name,
              mode);
    return true;
}

/**
 * @brief Imports a module into another module
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import
 * @return bool true if import was successful
 */
bool module_import(Module* target, const char* moduleName) {
    return module_import_with_options(target, moduleName, "", IMPORT_ALL);
}

/**
 * @brief Selectively imports symbols from a module
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import from
 * @param symbolNames Array of symbol names to import
 * @param aliases Array of aliases for the symbols (can be NULL)
 * @param count Number of symbols to import
 * @return bool true if import was successful
 */
bool module_import_symbols(Module* target, const char* moduleName, 
                          const char** symbolNames, const char** aliases, int count) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_import_symbols);
    
    if (!target || !moduleName || !symbolNames || count <= 0) {
        logger_log(LOG_ERROR, "Invalid parameters for selective import");
        error_report("Module", __LINE__, 0, "Invalid selective import parameters", ERROR_UNDEFINED);
        return false;
    }
    
    // First, import the module with selective mode
    if (!module_import_with_options(target, moduleName, "", IMPORT_SELECTIVE)) {
        return false;
    }
    
    // Find the import we just added
    ImportedModule* import = &target->imports[target->importCount - 1];
    
    // Allocate memory for the imported symbols
    import->symbols = calloc(count, sizeof(ImportedSymbol));
    if (!import->symbols) {
        logger_log(LOG_ERROR, "Failed to allocate memory for imported symbols");
        error_report("Module", __LINE__, 0, "Memory allocation failed", ERROR_MEMORY);
        return false;
    }
    
    import->symbolCount = count;
    
    // Add each symbol
    for (int i = 0; i < count; i++) {
        // Look up the symbol in the imported module
        ExportedSymbol* symbol = module_find_export(import->module, symbolNames[i]);
        if (!symbol) {
            char errMsg[1024];
            snprintf(errMsg, sizeof(errMsg), "Symbol '%s' not found in module '%s'", 
                    symbolNames[i], moduleName);
            logger_log(LOG_WARNING, "%s", errMsg);
            continue;
        }
        
        // Copy the symbol information
        strncpy(import->symbols[i].name, symbolNames[i], sizeof(import->symbols[i].name) - 1);
        strncpy(import->symbols[i].alias, 
                aliases && aliases[i] ? aliases[i] : symbolNames[i], 
                sizeof(import->symbols[i].alias) - 1);
        import->symbols[i].symbol = symbol;
        
        logger_log(LOG_DEBUG, "Imported symbol '%s'%s%s from module '%s'", 
                  symbolNames[i],
                  aliases && aliases[i] ? " as " : "",
                  aliases && aliases[i] ? aliases[i] : "",
                  moduleName);
    }
    
    return true;
}

/**
 * @brief Finds an exported symbol in a module
 * 
 * @param module The module to search in
 * @param name Name of the symbol to find
 * @return ExportedSymbol* Pointer to the found symbol, or NULL if not found
 */
ExportedSymbol* module_find_export(Module* module, const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_find_export);
    
    if (!module || !name) {
        return NULL;
    }
    
    for (int i = 0; i < module->exportCount; i++) {
        if (strcmp(module->exports[i].name, name) == 0) {
            // For internal visibility, we would need to check if the caller is in the same package
            // For now, we just treat internal as public for simplicity
            if (module->exports[i].visibility == EXPORT_PUBLIC || 
                module->exports[i].visibility == EXPORT_INTERNAL) {
                return &module->exports[i];
            } else {
                if (debug_level >= 3) {
                    logger_log(LOG_DEBUG, "Symbol '%s' found in module '%s' but has private visibility",
                              name, module->name);
                }
                return NULL;  // Symbol exists but is not accessible
            }
        }
    }
    
    return NULL;
}

/**
 * @brief Resolves a symbol name in a module
 * 
 * Searches for the symbol in the module's exports and unqualified imports.
 * 
 * @param module The module to search in
 * @param name Name of the symbol to resolve
 * @return AstNode* The AST node for the symbol, or NULL if not found
 */
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

    // 1. Search in local exports first
    ExportedSymbol* localSymbol = module_find_export(module, name);
    if (localSymbol) {
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Symbol '%s' found in exports of module '%s'", 
                      name, module->name);
        }
        return localSymbol->node;
    }

    // 2. Search in imports
    for (int i = 0; i < module->importCount; i++) {
        ImportedModule* import = &module->imports[i];
        
        // Handle different import modes
        switch (import->mode) {
            case IMPORT_ALL:
                // Check all exports of the imported module
                ExportedSymbol* importedSymbol = module_find_export(import->module, name);
                if (importedSymbol) {
                    if (debug_level >= 3) {
                        logger_log(LOG_DEBUG, "Symbol '%s' found in imported module '%s' (all mode)", 
                                  name, import->name);
                    }
                    return importedSymbol->node;
                }
                break;
                
            case IMPORT_SELECTIVE:
                // Check only explicitly imported symbols
                for (int j = 0; j < import->symbolCount; j++) {
                    if (strcmp(import->symbols[j].alias, name) == 0) {
                        if (debug_level >= 3) {
                            logger_log(LOG_DEBUG, "Symbol '%s' found as alias for '%s' in selective import from module '%s'",
                                      name, import->symbols[j].name, import->name);
                        }
                        return import->symbols[j].symbol->node;
                    }
                }
                break;
                
            case IMPORT_QUALIFIED:
                // Qualified imports need explicit qualification, so they're not checked here
                break;
                
            default:
                break;
        }
    }
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Symbol '%s' not found in module '%s' or its imports", 
                  name, module->name);
    }

    return NULL;
}

/**
 * @brief Resolves a qualified symbol name in a module
 * 
 * @param module The module to search in
 * @param moduleName Name of the module containing the symbol
 * @param symbolName Name of the symbol to resolve
 * @return AstNode* The AST node for the symbol, or NULL if not found
 */
AstNode* module_resolve_qualified_symbol(Module* module, const char* moduleName, const char* symbolName) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_resolve_qualified_symbol);
    
    if (!module || !moduleName || !symbolName) {
        return NULL;
    }
    
    // Find the imported module by name or alias
    for (int i = 0; i < module->importCount; i++) {
        ImportedModule* import = &module->imports[i];
        
        if (strcmp(import->name, moduleName) == 0 || 
            (import->alias[0] && strcmp(import->alias, moduleName) == 0)) {
            
            if (import->mode == IMPORT_SELECTIVE) {
                // For selective imports, check if the symbol was explicitly imported
                for (int j = 0; j < import->symbolCount; j++) {
                    if (strcmp(import->symbols[j].name, symbolName) == 0) {
                        return import->symbols[j].symbol->node;
                    }
                }
            } else {
                // For all other import modes, just look for the symbol in the module
                ExportedSymbol* symbol = module_find_export(import->module, symbolName);
                if (symbol) {
                    return symbol->node;
                }
            }
            
            logger_log(LOG_WARNING, "Symbol '%s' not found in module '%s'", 
                       symbolName, moduleName);
            return NULL;
        }
    }
    
    logger_log(LOG_WARNING, "Qualified symbol '%s.%s' not found in module '%s', module not imported", 
               moduleName, symbolName, module->name);
    return NULL;
}

/**
 * @brief Adds an export to a module
 * 
 * @param module The module to add the export to
 * @param name Name of the exported symbol
 * @param node AST node for the exported symbol
 * @param visibility Visibility level of the export
 */
void module_add_export(Module* module, const char* name, AstNode* node, ExportVisibility visibility) {
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

    // Check for duplicate
    for (int i = 0; i < module->exportCount; i++) {
        if (strcmp(module->exports[i].name, name) == 0) {
            logger_log(LOG_WARNING, "Symbol '%s' already exported in module '%s', overwriting", 
                     name, module->name);
            module->exports[i].node = node;
            module->exports[i].visibility = visibility;
            return;
        }
    }

    // Add new exported symbol
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
    newSymbol->type = NULL;  // Can be inferred/assigned later
    newSymbol->visibility = visibility;
    
    const char* visibilityStr;
    switch (visibility) {
        case EXPORT_PUBLIC: visibilityStr = "public"; break;
        case EXPORT_INTERNAL: visibilityStr = "internal"; break;
        case EXPORT_PRIVATE: visibilityStr = "private"; break;
        default: visibilityStr = "unknown"; break;
    }
    
    logger_log(LOG_DEBUG, "Symbol '%s' exported in module '%s' with visibility '%s'", 
              name, module->name, visibilityStr);
}

/**
 * @brief Sets the version of a module
 * 
 * @param module The module to set the version for
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 */
void module_set_version(Module* module, int major, int minor, int patch) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_version);
    
    if (!module) {
        logger_log(LOG_ERROR, "Attempted to set version for NULL module");
        error_report("Module", __LINE__, 0, "NULL module in set_version", ERROR_UNDEFINED);
        return;
    }
    
    module->version.major = major;
    module->version.minor = minor;
    module->version.patch = patch;
    
    logger_log(LOG_INFO, "Set version of module '%s' to %d.%d.%d", 
              module->name, major, minor, patch);
}

/**
 * @brief Sets metadata for a module
 * 
 * @param module The module to set metadata for
 * @param author Author name
 * @param description Module description
 * @param license License information
 */
void module_set_metadata(Module* module, const char* author, const char* description, const char* license) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_set_metadata);
    
    if (!module) {
        logger_log(LOG_ERROR, "Attempted to set metadata for NULL module");
        error_report("Module", __LINE__, 0, "NULL module in set_metadata", ERROR_UNDEFINED);
        return;
    }
    
    if (author) {
        strncpy(module->metadata.author, author, sizeof(module->metadata.author) - 1);
    }
    
    if (description) {
        strncpy(module->metadata.description, description, sizeof(module->metadata.description) - 1);
    }
    
    if (license) {
        strncpy(module->metadata.license, license, sizeof(module->metadata.license) - 1);
    }
    
    // Copy the version from the module to the metadata
    module->metadata.version = module->version;
    
    logger_log(LOG_INFO, "Updated metadata for module '%s'", module->name);
}

/**
 * @brief Prints detailed information about a module
 * 
 * @param module The module to print information about
 */
void module_print_info(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_print_info);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to print info of NULL module");
        return;
    }
    
    logger_log(LOG_INFO, "=== Module: %s ===", module->name);
    logger_log(LOG_INFO, "Path: %s", module->path);
    logger_log(LOG_INFO, "Version: %d.%d.%d", 
              module->version.major, module->version.minor, module->version.patch);
    logger_log(LOG_INFO, "Status: %s%s", 
              module->isLoaded ? "Loaded" : (module->isLoading ? "Loading" : "Unloaded"),
              module->isCached ? " (Cached)" : "");
    logger_log(LOG_INFO, "Exports: %d", module->exportCount);
    logger_log(LOG_INFO, "Imports: %d", module->importCount);
    logger_log(LOG_INFO, "Dependencies: %d", module->dependencyCount);

    if (module->metadata.author[0] || module->metadata.description[0] || module->metadata.license[0]) {
        logger_log(LOG_INFO, "Metadata:");
        if (module->metadata.author[0]) {
            logger_log(LOG_INFO, "  Author: %s", module->metadata.author);
        }
        if (module->metadata.description[0]) {
            logger_log(LOG_INFO, "  Description: %s", module->metadata.description);
        }
        if (module->metadata.license[0]) {
            logger_log(LOG_INFO, "  License: %s", module->metadata.license);
        }
    }

    if (debug_level >= 2) {
        // Log detailed export information
        logger_log(LOG_DEBUG, "Exports:");
        for (int i = 0; i < module->exportCount; i++) {
            const char* symbolType = "unknown";
            const char* visibilityStr;
            
            switch (module->exports[i].visibility) {
                case EXPORT_PUBLIC: visibilityStr = "public"; break;
                case EXPORT_INTERNAL: visibilityStr = "internal"; break;
                case EXPORT_PRIVATE: visibilityStr = "private"; break;
                default: visibilityStr = "unknown"; break;
            }
            
            if (module->exports[i].node) {
                switch (module->exports[i].node->type) {
                    case AST_FUNC_DEF: symbolType = "function"; break;
                    case AST_CLASS_DEF: symbolType = "class"; break;
                    case AST_VAR_DECL: symbolType = "variable"; break;
                    default: symbolType = "unknown"; break;
                }
            }
            
            logger_log(LOG_DEBUG, "  - %s (%s) [%s]", 
                      module->exports[i].name, 
                      symbolType,
                      visibilityStr);
        }
        
        // Log detailed import information
        logger_log(LOG_DEBUG, "Imports:");
        for (int i = 0; i < module->importCount; i++) {
            const char* modeStr;
            switch (module->imports[i].mode) {
                case IMPORT_ALL: modeStr = "all"; break;
                case IMPORT_SELECTIVE: modeStr = "selective"; break;
                case IMPORT_QUALIFIED: modeStr = "qualified"; break;
                default: modeStr = "unknown"; break;
            }
            
            logger_log(LOG_DEBUG, "  - %s%s%s (mode: %s)", 
                      module->imports[i].name,
                      module->imports[i].alias[0] ? " as " : "",
                      module->imports[i].alias[0] ? module->imports[i].alias : "",
                      modeStr);
                      
            // Log selective imports if any
            if (module->imports[i].mode == IMPORT_SELECTIVE && module->imports[i].symbolCount > 0) {
                for (int j = 0; j < module->imports[i].symbolCount; j++) {
                    logger_log(LOG_DEBUG, "    - %s%s%s", 
                              module->imports[i].symbols[j].name,
                              strcmp(module->imports[i].symbols[j].name, module->imports[i].symbols[j].alias) != 0 ? " as " : "",
                              strcmp(module->imports[i].symbols[j].name, module->imports[i].symbols[j].alias) != 0 ? module->imports[i].symbols[j].alias : "");
                }
            }
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
    printf("Version: %d.%d.%d\n", 
          module->version.major, module->version.minor, module->version.patch);
    printf("Status: %s%s\n", 
          module->isLoaded ? "Loaded" : (module->isLoading ? "Loading" : "Unloaded"),
          module->isCached ? " (Cached)" : "");
    printf("Exports: %d\n", module->exportCount);
    printf("Imports: %d\n", module->importCount);
    printf("Dependencies: %d\n", module->dependencyCount);
    
    if (module->metadata.author[0] || module->metadata.description[0] || module->metadata.license[0]) {
        printf("\nMetadata:\n");
        if (module->metadata.author[0]) {
            printf("  Author: %s\n", module->metadata.author);
        }
        if (module->metadata.description[0]) {
            printf("  Description: %s\n", module->metadata.description);
        }
        if (module->metadata.license[0]) {
            printf("  License: %s\n", module->metadata.license);
        }
    }
    
    printf("\nExports:\n");
    for (int i = 0; i < module->exportCount; i++) {
        const char* symbolType = "unknown";
        const char* visibilityStr;
        
        switch (module->exports[i].visibility) {
            case EXPORT_PUBLIC: visibilityStr = "public"; break;
            case EXPORT_INTERNAL: visibilityStr = "internal"; break;
            case EXPORT_PRIVATE: visibilityStr = "private"; break;
            default: visibilityStr = "unknown"; break;
        }
        
        if (module->exports[i].node) {
            switch (module->exports[i].node->type) {
                case AST_FUNC_DEF: symbolType = "function"; break;
                case AST_CLASS_DEF: symbolType = "class"; break;
                case AST_VAR_DECL: symbolType = "variable"; break;
                default: symbolType = "unknown"; break;
            }
        }
        
        printf("  - %s (%s) [%s]\n", 
              module->exports[i].name, 
              symbolType,
              visibilityStr);
    }
    
    printf("\nImports:\n");
    for (int i = 0; i < module->importCount; i++) {
        const char* modeStr;
        switch (module->imports[i].mode) {
            case IMPORT_ALL: modeStr = "all"; break;
            case IMPORT_SELECTIVE: modeStr = "selective"; break;
            case IMPORT_QUALIFIED: modeStr = "qualified"; break;
            default: modeStr = "unknown"; break;
        }
        
        printf("  - %s%s%s (mode: %s)\n", 
              module->imports[i].name,
              module->imports[i].alias[0] ? " as " : "",
              module->imports[i].alias[0] ? module->imports[i].alias : "",
              modeStr);
              
        // Print selective imports if any
        if (module->imports[i].mode == IMPORT_SELECTIVE && module->imports[i].symbolCount > 0) {
            for (int j = 0; j < module->imports[i].symbolCount; j++) {
                printf("    - %s%s%s\n", 
                      module->imports[i].symbols[j].name,
                      strcmp(module->imports[i].symbols[j].name, module->imports[i].symbols[j].alias) != 0 ? " as " : "",
                      strcmp(module->imports[i].symbols[j].name, module->imports[i].symbols[j].alias) != 0 ? module->imports[i].symbols[j].alias : "");
            }
        }
    }
    
    printf("\nDependencies:\n");
    for (int i = 0; i < module->dependencyCount; i++) {
        printf("  - %s\n", module->dependencies[i]);
    }
    
    printf("==================\n");
}

/**
 * @brief Gets the number of currently loaded modules
 * 
 * @return int Number of loaded modules
 */
int module_count_loaded(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_count_loaded);
    return moduleCount;
}

/**
 * @brief Gets the name of a module
 * 
 * @param module The module to get the name of
 * @return const char* The module's name, or "NULL" if module is NULL
 */
const char* module_get_name(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_get_name);
    
    if (!module) {
        logger_log(LOG_WARNING, "Attempted to get name of NULL module");
        return "NULL";
    }
    
    return module->name;
}

/**
 * @brief Imports a module with an optional alias (backward compatibility)
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import
 * @param alias Optional alias for the imported module
 * @param isQualified Whether this is a qualified import
 * @return bool true if import was successful
 */
bool module_import_with_alias(Module* target, const char* moduleName, const char* alias, bool isQualified) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_import_with_alias);
    
    ImportMode mode = isQualified ? IMPORT_QUALIFIED : IMPORT_ALL;
    return module_import_with_options(target, moduleName, alias, mode);
}

/**
 * @brief Carga dinámicamente un módulo desde una biblioteca compartida
 * 
 * @param module El módulo cuyas funciones se intentarán cargar dinámicamente
 * @return bool true si tuvo éxito, false si falló
 */
bool module_load_dynamic(Module* module) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)module_load_dynamic);
    
    if (!module) {
        logger_log(LOG_ERROR, "Attempted to load NULL module dynamically");
        return false;
    }
    
    char filename[1024];
    const char* extensions[] = {
    #ifdef _WIN32
        ".dll"
    #elif __APPLE__
        ".dylib", ".so"
    #else
        ".so", ".dylib"
    #endif
    };
    
    // Intentar diferentes extensiones
    DYNLIB_HANDLE handle = NULL;
    for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
        // Construir el nombre del archivo
        snprintf(filename, sizeof(filename), "%s%s", module->name, extensions[i]);
        
        // Intentar cargar la biblioteca
        handle = DYNLIB_LOAD(filename);
        if (handle) {
            break;
        }
        
        // Intentar en directorio de módulos
        snprintf(filename, sizeof(filename), "modules/%s%s", module->name, extensions[i]);
        handle = DYNLIB_LOAD(filename);
        if (handle) {
            break;
        }
        
        // Intentar en directorio lib
        snprintf(filename, sizeof(filename), "lib/%s%s", module->name, extensions[i]);
        handle = DYNLIB_LOAD(filename);
        if (handle) {
            break;
        }
    }
    
    if (!handle) {
        #ifdef _WIN32
        DWORD error = GetLastError();
        logger_log(LOG_WARNING, "Could not load module '%s' dynamically. Error: %lu", module->name, error);
        #else
        logger_log(LOG_WARNING, "Could not load module '%s' dynamically: %s", module->name, dlerror());
        #endif
        return false;
    }
    
    logger_log(LOG_INFO, "Successfully loaded module '%s' dynamically", module->name);
    
    // Obtener información del módulo
    typedef ModuleInfo* (*GetModuleInfoFunc)();
    GetModuleInfoFunc getInfo = (GetModuleInfoFunc)DYNLIB_GETSYM(handle, "getModuleInfo");
    
    if (getInfo) {
        ModuleInfo* info = getInfo();
        if (info) {
            // Actualizar la información del módulo
            module->version.major = info->version.major;
            module->version.minor = info->version.minor;
            module->version.patch = info->version.patch;
            
            if (info->author && strlen(info->author) > 0) {
                strncpy(module->metadata.author, info->author, sizeof(module->metadata.author) - 1);
            }
            
            if (info->description && strlen(info->description) > 0) {
                strncpy(module->metadata.description, info->description, sizeof(module->metadata.description) - 1);
            }
            
            if (info->license && strlen(info->license) > 0) {
                strncpy(module->metadata.license, info->license, sizeof(module->metadata.license) - 1);
            }
            
            logger_log(LOG_INFO, "Extracted module info: version %d.%d.%d, author: %s", 
                      info->version.major, info->version.minor, info->version.patch, 
                      info->author ? info->author : "unknown");
        }
    }
    
    // Cargar el AST del módulo (si está disponible)
    typedef AstNode* (*GetModuleAstFunc)();
    GetModuleAstFunc getAst = (GetModuleAstFunc)DYNLIB_GETSYM(handle, "getModuleAst");
    
    if (getAst) {
        AstNode* ast = getAst();
        if (ast) {
            // Si ya teníamos un AST, liberarlo primero
            if (module->ast) {
                freeAst(module->ast);
            }
            
            // Usar el AST del módulo dinámico
            module->ast = ast;
            logger_log(LOG_INFO, "Loaded AST from dynamic module '%s'", module->name);
        }
    }
    
    // Cargar los símbolos exportados
    typedef ExportDefinition* (*GetExportsFunc)(int*);
    GetExportsFunc getExports = (GetExportsFunc)DYNLIB_GETSYM(handle, "getExports");
    
    if (getExports) {
        int exportCount = 0;
        ExportDefinition* exports = getExports(&exportCount);
        
        if (exports && exportCount > 0) {
            for (int i = 0; i < exportCount; i++) {
                // Crear un nodo temporal para este símbolo
                AstNode* symbolNode = createAstNode(AST_IDENTIFIER);
                strncpy(symbolNode->identifier.name, exports[i].name, sizeof(symbolNode->identifier.name) - 1);
                
                // Agregar a las exportaciones del módulo
                module_add_export(module, exports[i].name, symbolNode, 
                                 exports[i].visibility == 1 ? EXPORT_PUBLIC : 
                                 (exports[i].visibility == 2 ? EXPORT_INTERNAL : EXPORT_PRIVATE));
                
                logger_log(LOG_DEBUG, "Added export '%s' from dynamic module", exports[i].name);
            }
            
            logger_log(LOG_INFO, "Loaded %d exports from dynamic module '%s'", exportCount, module->name);
        }
    }
    
    // Guardar el handle para usarlo más tarde
    module->dynamicHandle = handle;
    module->isDynamicallyLoaded = true;
    
    return true;
}
