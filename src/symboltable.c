#include "symboltable.h"
#include <stdlib.h>
#include <string.h>

SymbolTable* symbolTable_create(void) {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    
    table->head = NULL;
    table->currentScope = 0;
    
    return table;
}

void symbolTable_free(SymbolTable* table) {
    if (!table) return;
    
    Symbol* current = table->head;
    while (current) {
        Symbol* next = current->next;
        if (current->type) {
            freeType(current->type);
        }
        free(current);
        current = next;
    }
    
    free(table);
}

void symbolTable_enterScope(SymbolTable* table) {
    if (!table) return;
    table->currentScope++;
}

void symbolTable_exitScope(SymbolTable* table) {
    if (!table || table->currentScope <= 0) return;
    
    // Eliminar todos los símbolos del ámbito actual
    Symbol* current = table->head;
    Symbol* prev = NULL;
    
    while (current) {
        if (current->scope == table->currentScope) {
            Symbol* toDelete = current;
            
            if (prev) {
                prev->next = current->next;
                current = current->next;
            } else {
                table->head = current->next;
                current = table->head;
            }
            
            freeType(toDelete->type);
            free(toDelete);
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    table->currentScope--;
}

void symbolTable_add(SymbolTable* table, const char* name, Type* type) {
    if (!table || !name || !type) return;
    
    // Crear nuevo símbolo
    Symbol* symbol = malloc(sizeof(Symbol));
    if (!symbol) return;
    
    strncpy(symbol->name, name, sizeof(symbol->name) - 1);
    symbol->name[sizeof(symbol->name) - 1] = '\0';
    symbol->type = clone_type(type);
    symbol->scope = table->currentScope;
    
    // Insertar al principio de la lista
    symbol->next = table->head;
    table->head = symbol;
}

Symbol* symbolTable_lookup(SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

Symbol* symbolTable_lookupCurrentScope(SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0 && current->scope == table->currentScope) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}
