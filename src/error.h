#ifndef LYN_ERROR_H
#define LYN_ERROR_H

#include <stddef.h>

// Agregar la enumeración de tipos de error
typedef enum {
    ERROR_NONE,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_TYPE,
    ERROR_MEMORY,
    ERROR_IO,
    ERROR_UNDEFINED,
    ERROR_RUNTIME
} ErrorType;

typedef struct {
    int line;
    int column;
    const char* file;
    const char* message;
    const char* context;     // Línea de código donde ocurrió el error
    int contextLength;       // Longitud del contexto
    int errorPosition;       // Posición del error en el contexto
} ErrorInfo;

typedef enum {
    WARNING_UNUSED_VAR,
    WARNING_SHADOWING,
    WARNING_IMPLICIT_CONVERSION
    // ...etc
} WarningType;

// Reporta un error y establece el mensaje de error actual.
void error_report(const char* module, int line, int col, const char* message);

// Establece el código fuente actual para el sistema de errores.
void error_set_source(const char* source);

// Imprime el mensaje de error actual con contexto.
void error_print_current(void);

#endif /* ERROR_H */
