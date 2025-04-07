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
#include <time.h>

/**
 * @brief Visibility level for exported symbols
 */
typedef enum ExportVisibility {
    EXPORT_PRIVATE,    ///< Symbol is only visible within the module
    EXPORT_INTERNAL,   ///< Symbol is visible to modules in the same package
    EXPORT_PUBLIC      ///< Symbol is visible to all modules
} ExportVisibility;

/**
 * @brief Import mode for module imports
 */
typedef enum ImportMode {
    IMPORT_ALL,        ///< Import the entire module
    IMPORT_SELECTIVE,  ///< Import only specified symbols
    IMPORT_QUALIFIED   ///< Import with namespace qualification
} ImportMode;

/**
 * @brief Structure representing an exported symbol
 */
typedef struct ExportedSymbol {
    char name[256];             ///< Name of the exported symbol
    AstNode* node;              ///< Corresponding AST node
    Type* type;                 ///< Type of the symbol (optional)
    ExportVisibility visibility; ///< Visibility level of the symbol
} ExportedSymbol;

/**
 * @brief Structure representing a selectively imported symbol
 */
typedef struct ImportedSymbol {
    char name[256];             ///< Original name of the imported symbol
    char alias[256];            ///< Alias for the symbol (if any)
    ExportedSymbol* symbol;     ///< Pointer to the actual exported symbol
} ImportedSymbol;

/**
 * @brief Structure representing an imported module
 */
typedef struct ImportedModule {
    char name[256];              ///< Name of the imported module
    char alias[256];             ///< Alias used (can be empty)
    ImportMode mode;             ///< Import mode
    struct Module* module;       ///< Pointer to the imported module
    ImportedSymbol* symbols;     ///< Array of selectively imported symbols
    int symbolCount;             ///< Number of selectively imported symbols
} ImportedModule;

/**
 * @brief Structure for module version information
 */
typedef struct ModuleVersion {
    int major;                  ///< Major version number
    int minor;                  ///< Minor version number
    int patch;                  ///< Patch version number
} ModuleVersion;

/**
 * @brief Module metadata structure
 */
typedef struct ModuleMetadata {
    char author[256];           ///< Author name
    char description[1024];     ///< Module description
    ModuleVersion version;      ///< Module version
    char license[256];          ///< License information
} ModuleMetadata;

/**
 * @brief Structure representing a module
 */
typedef struct Module {
    char name[256];             ///< Name of the module
    char path[1024];            ///< Path to the module file
    
    // Namespace and exports system
    ExportedSymbol* exports;    ///< Exported symbols
    int exportCount;            ///< Number of exported symbols
    
    // Imports system
    ImportedModule* imports;    ///< Imported modules
    int importCount;            ///< Number of imported modules
    
    // Dependencies
    char** dependencies;        ///< List of dependent module names
    int dependencyCount;        ///< Number of dependencies
    
    // Module state
    bool isLoaded;              ///< Whether the module has been fully loaded
    bool isLoading;             ///< Used for circular dependency detection
    
    // Cache information
    time_t lastModified;        ///< Last modification time of the source file
    bool isCached;              ///< Whether the module is cached
    
    // Versioning
    ModuleVersion version;      ///< Module version
    ModuleMetadata metadata;    ///< Module metadata
    
    AstNode* ast;               ///< AST of the module
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
 * @brief Loads a module with caching
 * 
 * @param name Name of the module to load
 * @param forceFresh Whether to force a fresh load ignoring cache
 * @return Module* Pointer to the loaded module, or NULL if loading failed
 */
Module* module_load_cached(const char* name, bool forceFresh);

/**
 * @brief Reloads a module if its source file has changed
 * 
 * @param module The module to check and potentially reload
 * @return bool true if the module was reloaded
 */
bool module_hot_reload(Module* module);

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
 * @param mode Import mode (all, selective, qualified)
 * @return bool true if import was successful
 */
bool module_import_with_options(Module* target, const char* moduleName, const char* alias, ImportMode mode);

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
                          const char** symbolNames, const char** aliases, int count);

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
 * @param visibility Visibility level of the export
 */
void module_add_export(Module* module, const char* name, AstNode* node, ExportVisibility visibility);

/**
 * @brief Sets the version of a module
 * 
 * @param module The module to set the version for
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 */
void module_set_version(Module* module, int major, int minor, int patch);

/**
 * @brief Sets metadata for a module
 * 
 * @param module The module to set metadata for
 * @param author Author name
 * @param description Module description
 * @param license License information
 */
void module_set_metadata(Module* module, const char* author, const char* description, const char* license);

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