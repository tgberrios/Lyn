/**
 * @file module.h
 * @brief Module system interface for the Lyn compiler
 * 
 * This header file defines the module system interface, which provides:
 * - Module loading and management
 * - Symbol export/import functionality
 * - Dependency tracking
 * - Namespace support
 */

#ifndef LYN_MODULE_H
#define LYN_MODULE_H

#include "ast.h"
#include "error.h"
#include "logger.h"
#include "types.h"  // Added for access to the Type structure
#include <stdbool.h>

/**
 * @brief Structure representing an exported symbol
 */
typedef struct ExportedSymbol {
    char name[256];         ///< Name of the exported symbol
    AstNode* node;          ///< Corresponding AST node
    Type* type;             ///< Type of the symbol (optional)
    bool isPublic;          ///< Whether the symbol is public or internal
} ExportedSymbol;

/**
 * @brief Structure representing an imported module
 */
typedef struct ImportedModule {
    char name[256];         ///< Name of the imported module
    char alias[256];        ///< Alias used (can be empty)
    bool isQualified;       ///< Whether the import is qualified (requires namespace)
    struct Module* module;  ///< Pointer to the imported module
} ImportedModule;

/**
 * @brief Structure representing a module
 */
typedef struct Module {
    char name[256];         ///< Name of the module
    char path[1024];        ///< Path to the module file
    
    // Namespace and exports system
    ExportedSymbol* exports; ///< Exported symbols
    int exportCount;        ///< Number of exported symbols
    
    // Imports system
    ImportedModule* imports; ///< Imported modules
    int importCount;        ///< Number of imported modules
    
    // Dependencies
    char** dependencies;    ///< List of dependent module names
    int dependencyCount;    ///< Number of dependencies
    
    bool isLoaded;          ///< Whether the module has been fully loaded
    bool isLoading;         ///< Used for circular dependency detection
    
    AstNode* ast;           ///< AST of the module
} Module;

/**
 * @brief Initializes the module system
 */
void module_system_init(void);

/**
 * @brief Cleans up the module system
 */
void module_system_cleanup(void);

/**
 * @brief Sets the debug level for the module system
 * 
 * @param level Debug level (0=minimum, 3=maximum)
 */
void module_set_debug_level(int level);

/**
 * @brief Gets the current debug level for the module system
 * 
 * @return int Current debug level (0=minimum, 3=maximum)
 */
int module_get_debug_level(void);

/**
 * @brief Loads a module from disk
 * 
 * @param name Name of the module to load
 * @return Module* Pointer to the loaded module, or NULL if loading failed
 */
Module* module_load(const char* name);

/**
 * @brief Imports a module into another module
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import
 * @return bool true if import was successful
 */
bool module_import(Module* target, const char* moduleName);

/**
 * @brief Imports a module with an optional alias
 * 
 * @param target The module to import into
 * @param moduleName Name of the module to import
 * @param alias Optional alias for the imported module
 * @param isQualified Whether this is a qualified import
 * @return bool true if import was successful
 */
bool module_import_with_alias(Module* target, const char* moduleName, const char* alias, bool isQualified);

/**
 * @brief Resolves a symbol name in a module
 * 
 * @param module The module to search in
 * @param name Name of the symbol to resolve
 * @return AstNode* The AST node for the symbol, or NULL if not found
 */
AstNode* module_resolve_symbol(Module* module, const char* name);

/**
 * @brief Resolves a qualified symbol name in a module
 * 
 * @param module The module to search in
 * @param moduleName Name of the module containing the symbol
 * @param symbolName Name of the symbol to resolve
 * @return AstNode* The AST node for the symbol, or NULL if not found
 */
AstNode* module_resolve_qualified_symbol(Module* module, const char* moduleName, const char* symbolName);

/**
 * @brief Adds an export to a module
 * 
 * @param module The module to add the export to
 * @param name Name of the exported symbol
 * @param node AST node for the exported symbol
 * @param isPublic Whether the export is public
 */
void module_add_export(Module* module, const char* name, AstNode* node, bool isPublic);

/**
 * @brief Prints detailed information about a module
 * 
 * @param module The module to print information about
 */
void module_print_info(Module* module);

/**
 * @brief Detects circular dependencies between modules
 * 
 * @param module The module being checked
 * @param dependencyName Name of the potential dependency
 * @return bool true if a circular dependency is detected
 */
bool module_detect_circular_dependency(Module* module, const char* dependencyName);

/**
 * @brief Adds a dependency to a module
 * 
 * @param module The module to add the dependency to
 * @param dependencyName Name of the dependency to add
 */
void module_add_dependency(Module* module, const char* dependencyName);

/**
 * @brief Finds an exported symbol in a module
 * 
 * @param module The module to search in
 * @param name Name of the symbol to find
 * @return ExportedSymbol* Pointer to the found symbol, or NULL if not found
 */
ExportedSymbol* module_find_export(Module* module, const char* name);

/**
 * @brief Gets the number of currently loaded modules
 * 
 * @return int Number of loaded modules
 */
int module_count_loaded(void);

/**
 * @brief Gets the name of a module
 * 
 * @param module The module to get the name of
 * @return const char* The module's name, or "NULL" if module is NULL
 */
const char* module_get_name(Module* module);

/**
 * @brief Gets a module by name
 * 
 * @param name Name of the module to get
 * @return Module* Pointer to the module, or NULL if not found
 */
Module* module_get_by_name(const char* name);

/**
 * @brief Sets the module search paths
 * 
 * @param paths Array of search path strings
 * @param count Number of paths in the array
 */
void module_set_search_paths(const char* paths[], int count);

#endif // LYN_MODULE_H