/**
 * @file symboltable.h
 * @brief Header file for the symbol table implementation
 * 
 * This header defines the interface for the symbol table component of the Lyn compiler.
 * The symbol table manages variable and function declarations during compilation,
 * providing scope management and symbol lookup capabilities.
 * 
 * Features:
 * - Hierarchical scope management
 * - Symbol lookup in current and outer scopes
 * - Type checking and validation
 * - Debug information and logging
 */

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "error.h"
#include "logger.h"

/** Forward declaration of the Type structure */
typedef struct Type Type;

/**
 * @brief Structure representing a symbol in the symbol table
 * 
 * Each symbol contains:
 * - Name of the variable or function
 * - Type information
 * - Scope level
 * - Link to next symbol in the list
 */
typedef struct Symbol {
    char name[256];      ///< Name of the symbol
    Type* type;         ///< Type information
    int scope;          ///< Scope level where the symbol is defined
    struct Symbol* next; ///< Pointer to next symbol in the list
} Symbol;

/**
 * @brief Structure representing the symbol table
 * 
 * The symbol table is implemented as a linked list of symbols,
 * with tracking of the current scope level.
 */
typedef struct SymbolTable {
    Symbol* head;        ///< Pointer to first symbol in the list
    int currentScope;    ///< Current scope level (0 is global scope)
} SymbolTable;

// Include types.h after our definitions to avoid circular references
#include "types.h"

/**
 * @brief Creates a new symbol table
 * 
 * Allocates and initializes a new symbol table with an empty symbol list
 * and scope level 0 (global scope).
 * 
 * @return SymbolTable* Newly created symbol table, or NULL on allocation failure
 */
SymbolTable* symbolTable_create(void);

/**
 * @brief Frees all memory associated with a symbol table
 * 
 * Recursively frees all symbols in the table and their associated types,
 * then frees the table structure itself.
 * 
 * @param table The symbol table to free
 */
void symbolTable_free(SymbolTable* table);

/**
 * @brief Enters a new scope level
 * 
 * Increments the current scope level. All symbols added after this call
 * will be associated with the new scope level.
 * 
 * @param table The symbol table to modify
 */
void symbolTable_enterScope(SymbolTable* table);

/**
 * @brief Exits the current scope level
 * 
 * Removes all symbols from the current scope and decrements the scope level.
 * Cannot exit the global scope (level 0).
 * 
 * @param table The symbol table to modify
 */
void symbolTable_exitScope(SymbolTable* table);

/**
 * @brief Adds a new symbol to the current scope
 * 
 * Creates and adds a new symbol with the given name and type to the current scope.
 * Checks for duplicate symbols in the current scope and validates input parameters.
 * 
 * @param table The symbol table to modify
 * @param name The name of the new symbol
 * @param type The type of the new symbol
 */
void symbolTable_add(SymbolTable* table, const char* name, Type* type);

/**
 * @brief Looks up a symbol in all scopes
 * 
 * Searches for a symbol with the given name in all scopes, starting from
 * the current scope and moving outward to the global scope.
 * 
 * @param table The symbol table to search
 * @param name The name of the symbol to find
 * @return Symbol* The found symbol, or NULL if not found
 */
Symbol* symbolTable_lookup(SymbolTable* table, const char* name);

/**
 * @brief Looks up a symbol in the current scope only
 * 
 * Searches for a symbol with the given name only in the current scope.
 * 
 * @param table The symbol table to search
 * @param name The name of the symbol to find
 * @return Symbol* The found symbol, or NULL if not found
 */
Symbol* symbolTable_lookupCurrentScope(SymbolTable* table, const char* name);

/**
 * @brief Sets the debug level for symbol table operations
 * 
 * Controls the verbosity of logging for symbol table operations:
 * - 0: Minimal logging
 * - 1: Basic operations
 * - 2: Detailed operations
 * - 3: Verbose debugging
 * 
 * @param level New debug level (0-3)
 */
void symbolTable_set_debug_level(int level);

/**
 * @brief Gets the total number of symbols in the table
 * 
 * @param table The symbol table to count symbols in
 * @return int The total number of symbols
 */
int symbolTable_get_count(SymbolTable* table);

/**
 * @brief Prints a detailed dump of the symbol table
 * 
 * Outputs all symbols in the table with their names, types, and scope levels.
 * The output is sent to both the logger and stdout (if debug level is high enough).
 * 
 * @param table The symbol table to dump
 */
void symbolTable_dump(SymbolTable* table);

#endif /* SYMBOLTABLE_H */
