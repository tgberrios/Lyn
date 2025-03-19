#ifndef LYN_ERROR_H
#define LYN_ERROR_H

#include <stddef.h>

// Enumeración de tipos de error
typedef enum {
    ERROR_NONE = 0,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_TYPE,
    ERROR_NAME,     // Add missing error constant
    ERROR_MEMORY,
    ERROR_IO,
    ERROR_LIMIT,    // Add missing error constant
    ERROR_UNDEFINED,
    ERROR_RUNTIME,
    ERROR_MAX
} ErrorType;

typedef struct {
    int line;
    int column;
    const char* file;
    char* message;
    char* context;     // Línea de código donde ocurrió el error
    int contextLength; // Longitud del contexto
    int errorPosition; // Posición del error en el contexto
    ErrorType type;
} ErrorInfo;

// Reporta un error y almacena el mensaje actual, incluyendo detalles de ubicación y tipo.
void error_report(const char* module, int line, int col, const char* message, ErrorType type);

// Establece el código fuente actual para que el sistema de errores pueda extraer el contexto.
void error_set_source(const char* source);

// Imprime el error actual con contexto y stack trace.
void error_print_current(void);

// Funciones auxiliares para obtener información del error.
int error_get_count(void);
const ErrorInfo* error_get_last(void);

// Obtiene un mensaje descriptivo según el tipo de error.
const char* get_error_message(ErrorType type);

// Push debug information for stack trace
void error_push_debug(const char* func, const char* file, int line, void* addr);

#endif /* LYN_ERROR_H */
