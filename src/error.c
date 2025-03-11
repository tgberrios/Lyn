#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX_ERRORS 100
#define CONTEXT_SIZE 80
#define MAX_ERROR_LEN 1024

static ErrorInfo errors[MAX_ERRORS];
static int errorCount = 0;
static char errorBuffer[MAX_ERROR_LEN];
static const char* sourceCode = NULL;
static int errorLine = 0;
static int errorCol = 0;
static ErrorType errorType = ERROR_NONE;

void error_init(void) {
    errorCount = 0;
    sourceCode = NULL;
}

static void extract_context(ErrorInfo* error) {
    if (!sourceCode) return;

    // Encuentra el inicio de la línea
    const char* lineStart = sourceCode;
    const char* current = sourceCode;
    int currentLine = 1;
    
    while (*current && currentLine < error->line) {
        if (*current == '\n') {
            currentLine++;
            lineStart = current + 1;
        }
        current++;
    }

    // Encuentra el fin de la línea
    const char* lineEnd = lineStart;
    while (*lineEnd && *lineEnd != '\n') lineEnd++;

    // Calcula la longitud del contexto
    error->contextLength = (int)(lineEnd - lineStart);
    if (error->contextLength > CONTEXT_SIZE) {
        error->contextLength = CONTEXT_SIZE;
    }

    // Copia el contexto
    error->context = malloc(error->contextLength + 1);
    strncpy((char*)error->context, lineStart, error->contextLength);
    ((char*)error->context)[error->contextLength] = '\0';

    // Calcula la posición del error en el contexto
    error->errorPosition = error->column - 1;
}

void error_report(const char* module, int line, int col, const char* message) {
    if (errorCount >= MAX_ERRORS) return;

    ErrorInfo* error = &errors[errorCount++];
    error->file = module;
    error->line = line;
    error->column = col;
    error->message = strdup(message);
    
    extract_context(error);

    snprintf(errorBuffer, MAX_ERROR_LEN, "[%s:%d:%d] %s", 
             module, line, col, message);
    errorLine = line;
    errorCol = col;
    errorType = ERROR_SYNTAX;
}

void error_set_source(const char* source) {
    sourceCode = source;
}

void error_print_current(void) {
    if (errorCount == 0) return;

    ErrorInfo* error = &errors[errorCount - 1];
    fprintf(stderr, "\n=== Error en %s:%d:%d ===\n", 
            error->file, error->line, error->column);
    fprintf(stderr, "Error: %s\n", error->message);
    
    if (error->context) {
        fprintf(stderr, "\n%s\n", error->context);
        for (int i = 0; i < error->errorPosition; i++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n");
    }

    if (errorBuffer[0] != '\0') {
        fprintf(stderr, "ERROR: %s\n", errorBuffer);
        
        // Si tenemos el código fuente y la posición, mostrar contexto
        if (sourceCode && errorLine > 0) {
            // Encontrar la línea correcta
            int curLine = 1;
            const char* ptr = sourceCode;
            while (curLine < errorLine && *ptr) {
                if (*ptr == '\n') curLine++;
                ptr++;
            }
            
            // Encontrar el inicio de la línea
            const char* lineStart = ptr;
            while (lineStart > sourceCode && *(lineStart-1) != '\n') {
                lineStart--;
            }
            
            // Encontrar el final de la línea
            const char* lineEnd = ptr;
            while (*lineEnd && *lineEnd != '\n') lineEnd++;
            
            // Mostrar la línea
            fprintf(stderr, "Line %d: ", errorLine);
            for (const char* p = lineStart; p < lineEnd; p++) {
                putc(*p, stderr);
            }
            fprintf(stderr, "\n");
            
            // Mostrar el indicador de posición
            fprintf(stderr, "       ");
            for (int i = 1; i < errorCol; i++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "^\n");
        }
    }
}

int error_get_count(void) {
    return errorCount;
}

const ErrorInfo* error_get_last(void) {
    return errorCount > 0 ? &errors[errorCount - 1] : NULL;
}

// Agregar mensajes más descriptivos y específicos
const char* get_error_message(ErrorType type) {
    switch(type) {
        case ERROR_SYNTAX:
            return "Error de sintaxis";
        case ERROR_TYPE:
            return "Error de tipo";
        case ERROR_UNDEFINED:
            return "Variable o función no definida";
        // ...etc
    }
}
