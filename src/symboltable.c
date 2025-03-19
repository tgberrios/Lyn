#include "symboltable.h"
#include "error.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Nivel de depuración (0=mínimo, 3=máximo)
static int debug_level = 1;

void symbolTable_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Symbol table debug level set to %d", level);
}

SymbolTable* symbolTable_create(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_create);
    
    SymbolTable* table = malloc(sizeof(SymbolTable));
    if (!table) {
        error_report("SymbolTable", __LINE__, 0, "Failed to allocate memory for symbol table", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Memory allocation failed for symbol table");
        return NULL;
    }
    
    table->head = NULL;
    table->currentScope = 0;
    
    logger_log(LOG_DEBUG, "Symbol table created");
    return table;
}

void symbolTable_free(SymbolTable* table) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_free);
    
    if (!table) {
        logger_log(LOG_WARNING, "Attempt to free NULL symbol table");
        return;
    }
    
    int count = 0;
    Symbol* current = table->head;
    while (current) {
        Symbol* next = current->next;
        if (current->type) {
            freeType(current->type);
        }
        free(current);
        current = next;
        count++;
    }
    
    logger_log(LOG_DEBUG, "Symbol table freed (%d symbols)", count);
    free(table);
}

void symbolTable_enterScope(SymbolTable* table) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_enterScope);
    
    if (!table) {
        logger_log(LOG_WARNING, "Attempt to enter scope on NULL symbol table");
        return;
    }
    
    table->currentScope++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Entered scope %d", table->currentScope);
    }
}

void symbolTable_exitScope(SymbolTable* table) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_exitScope);
    
    if (!table) {
        logger_log(LOG_WARNING, "Attempt to exit scope on NULL symbol table");
        return;
    }
    
    if (table->currentScope <= 0) {
        error_report("SymbolTable", __LINE__, 0, "Attempt to exit global scope", ERROR_SEMANTIC);
        logger_log(LOG_ERROR, "Cannot exit global scope (scope %d)", table->currentScope);
        return;
    }
    
    // Eliminar todos los símbolos del ámbito actual
    Symbol* current = table->head;
    Symbol* prev = NULL;
    int removed = 0;
    
    while (current) {
        if (current->scope == table->currentScope) {
            Symbol* toDelete = current;
            
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Removing symbol '%s' from scope %d", 
                          current->name, current->scope);
            }
            
            if (prev) {
                prev->next = current->next;
                current = current->next;
            } else {
                table->head = current->next;
                current = table->head;
            }
            
            freeType(toDelete->type);
            free(toDelete);
            removed++;
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    table->currentScope--;
    
    if (debug_level >= 1) {
        logger_log(LOG_DEBUG, "Exited scope %d (removed %d symbols)", 
                  table->currentScope + 1, removed);
    }
}

void symbolTable_add(SymbolTable* table, const char* name, Type* type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_add);
    
    if (!table) {
        logger_log(LOG_ERROR, "Attempt to add symbol to NULL table");
        return;
    }
    
    if (!name || !name[0]) {
        error_report("SymbolTable", __LINE__, 0, "Invalid symbol name (empty or NULL)", ERROR_SEMANTIC);
        logger_log(LOG_ERROR, "Cannot add symbol with empty name");
        return;
    }
    
    if (!type) {
        error_report("SymbolTable", __LINE__, 0, "NULL type for symbol", ERROR_TYPE);
        logger_log(LOG_ERROR, "Cannot add symbol '%s' with NULL type", name);
        return;
    }
    
    // Verificar si ya existe en el ámbito actual
    Symbol* existing = symbolTable_lookupCurrentScope(table, name);
    if (existing) {
        char errorMsg[512];
        snprintf(errorMsg, sizeof(errorMsg), 
                "Symbol '%s' already defined in current scope", name);
        error_report("SymbolTable", __LINE__, 0, errorMsg, ERROR_SEMANTIC);
        logger_log(LOG_WARNING, "%s", errorMsg);
        return;
    }
    
    // Crear nuevo símbolo
    Symbol* symbol = malloc(sizeof(Symbol));
    if (!symbol) {
        error_report("SymbolTable", __LINE__, 0, "Memory allocation failed for symbol", ERROR_MEMORY);
        logger_log(LOG_ERROR, "Failed to allocate memory for symbol '%s'", name);
        return;
    }
    
    strncpy(symbol->name, name, sizeof(symbol->name) - 1);
    symbol->name[sizeof(symbol->name) - 1] = '\0';
    symbol->type = clone_type(type);
    symbol->scope = table->currentScope;
    
    // Insertar al principio de la lista
    symbol->next = table->head;
    table->head = symbol;
    
    if (debug_level >= 1) {
        logger_log(LOG_DEBUG, "Added symbol '%s' of type '%s' to scope %d", 
                  name, typeToString(type), table->currentScope);
    }
}

Symbol* symbolTable_lookup(SymbolTable* table, const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_lookup);
    
    if (!table) {
        logger_log(LOG_WARNING, "Lookup on NULL symbol table");
        return NULL;
    }
    
    if (!name || !name[0]) {
        logger_log(LOG_WARNING, "Lookup with empty name");
        return NULL;
    }
    
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Found symbol '%s' in scope %d", 
                          name, current->scope);
            }
            return current;
        }
        current = current->next;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Symbol '%s' not found in any scope", name);
    }
    return NULL;
}

Symbol* symbolTable_lookupCurrentScope(SymbolTable* table, const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_lookupCurrentScope);
    
    if (!table) {
        logger_log(LOG_WARNING, "Current scope lookup on NULL symbol table");
        return NULL;
    }
    
    if (!name || !name[0]) {
        logger_log(LOG_WARNING, "Current scope lookup with empty name");
        return NULL;
    }
    
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0 && current->scope == table->currentScope) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "Found symbol '%s' in current scope %d", 
                          name, table->currentScope);
            }
            return current;
        }
        current = current->next;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Symbol '%s' not found in current scope %d", 
                  name, table->currentScope);
    }
    return NULL;
}

int symbolTable_get_count(SymbolTable* table) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_get_count);
    
    if (!table) {
        logger_log(LOG_WARNING, "Get count on NULL symbol table");
        return 0;
    }
    
    int count = 0;
    Symbol* current = table->head;
    while (current) {
        count++;
        current = current->next;
    }
    
    return count;
}

void symbolTable_dump(SymbolTable* table) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)symbolTable_dump);
    
    if (!table) {
        logger_log(LOG_WARNING, "Attempt to dump NULL symbol table");
        return;
    }
    
    logger_log(LOG_INFO, "Symbol Table Dump (current scope: %d)", table->currentScope);
    logger_log(LOG_INFO, "---------------------------------------");
    
    Symbol* current = table->head;
    int count = 0;
    
    while (current) {
        logger_log(LOG_INFO, "Symbol: %-20s | Type: %-12s | Scope: %d", 
                  current->name, 
                  current->type ? typeToString(current->type) : "NULL", 
                  current->scope);
        current = current->next;
        count++;
    }
    
    logger_log(LOG_INFO, "---------------------------------------");
    logger_log(LOG_INFO, "Total symbols: %d", count);
    
    // Si el nivel de debug es alto, también imprimimos en stdout para consola
    if (debug_level >= 2) {
        printf("Symbol Table Dump (current scope: %d)\n", table->currentScope);
        printf("---------------------------------------\n");
        
        current = table->head;
        
        while (current) {
            printf("Symbol: %-20s | Type: %-12s | Scope: %d\n", 
                  current->name, 
                  current->type ? typeToString(current->type) : "NULL", 
                  current->scope);
            current = current->next;
        }
        
        printf("---------------------------------------\n");
        printf("Total symbols: %d\n", count);
    }
}
