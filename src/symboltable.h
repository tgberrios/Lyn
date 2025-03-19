#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "error.h"
#include "logger.h"

// Forward declaration
typedef struct Type Type;

// Estructura para símbolos en la tabla
typedef struct Symbol {
    char name[256];
    Type* type;
    int scope;
    struct Symbol* next;
} Symbol;

// Tabla de símbolos implementada como lista enlazada
typedef struct SymbolTable {
    Symbol* head;
    int currentScope;
} SymbolTable;

// Incluir types.h después de nuestras definiciones para evitar referencias circulares
#include "types.h"

// Crear una tabla de símbolos
SymbolTable* symbolTable_create(void);

// Liberar la tabla de símbolos
void symbolTable_free(SymbolTable* table);

// Entrar a un nuevo ámbito
void symbolTable_enterScope(SymbolTable* table);

// Salir del ámbito actual
void symbolTable_exitScope(SymbolTable* table);

// Agregar un símbolo a la tabla
void symbolTable_add(SymbolTable* table, const char* name, Type* type);

// Buscar un símbolo en cualquier ámbito
Symbol* symbolTable_lookup(SymbolTable* table, const char* name);

// Buscar un símbolo solo en el ámbito actual
Symbol* symbolTable_lookupCurrentScope(SymbolTable* table, const char* name);

// Establece el nivel de depuración para la tabla de símbolos
void symbolTable_set_debug_level(int level);

// Obtiene el conteo de símbolos en la tabla
int symbolTable_get_count(SymbolTable* table);

// Imprime todos los símbolos con sus tipos y ámbitos
void symbolTable_dump(SymbolTable* table);

#endif // SYMBOLTABLE_H
