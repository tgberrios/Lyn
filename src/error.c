#define _GNU_SOURCE         /* Para habilitar Dl_info en dlfcn.h */
#include <dlfcn.h>         /* Debe ir primero para que Dl_info esté disponible */
#include "error.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <ctype.h>

#define MAX_ERRORS      100
#define CONTEXT_SIZE    120
#define STACK_MAX_DEPTH 16

// ANSI color codes (opcional, se pueden eliminar si se prefiere sin color)
#define COLOR_RESET     "\x1b[0m"
#define COLOR_RED       "\x1b[31m"
#define COLOR_CYAN      "\x1b[36m"
#define COLOR_YELLOW    "\x1b[33m"

// Estructura para debugging info
typedef struct {
    const char* function;
    const char* file;
    int line;
    void* address;
} DebugInfo;

static ErrorInfo errors[MAX_ERRORS];
static int errorCount = 0;
static const char* sourceCode = NULL;

// Añadir info de debugging al ErrorInfo
static DebugInfo debugStack[STACK_MAX_DEPTH];
static int debugStackDepth = 0;

// Internal: print a short stack trace (3 frames max)
static void print_stack_short(void) {
    void* buffer[STACK_MAX_DEPTH];
    int frames = backtrace(buffer, STACK_MAX_DEPTH);
    char** symbols = backtrace_symbols(buffer, frames);
    
    fprintf(stderr, COLOR_CYAN "Stack Trace:" COLOR_RESET "\n");
    
    for (int i = 1; i < frames && i < 5; i++) {
        Dl_info info;
        if (dladdr(buffer[i], &info) && info.dli_sname) {
            // Encontrar debug info correspondiente
            for (int j = 0; j < debugStackDepth; j++) {
                if (debugStack[j].address == buffer[i]) {
                    fprintf(stderr, "    %s%s%s at %s:%d\n",
                            COLOR_YELLOW, debugStack[j].function, COLOR_RESET,
                            debugStack[j].file, debugStack[j].line);
                    goto next_frame;
                }
            }
            fprintf(stderr, "    %s%s%s\n", 
                    COLOR_YELLOW, info.dli_sname, COLOR_RESET);
        } else {
            fprintf(stderr, "    %s\n", symbols[i]);
        }
next_frame:
        continue;
    }
    
    free(symbols);
}

// Internal: extract source context from sourceCode
static void extract_context(ErrorInfo* e) {
    if (!sourceCode) return;
    
    // Buscar el inicio de la línea anterior
    const char* start = sourceCode;
    const char* p = sourceCode;
    int ln = 1;
    int targetLine = e->line > 1 ? e->line - 1 : e->line;
    
    while (*p && ln < targetLine) {
        if (*p == '\n') { ln++; start = p + 1; }
        p++;
    }

    // Capturar 3 líneas de contexto
    char buffer[CONTEXT_SIZE * 3 + 1] = {0};
    int pos = 0;
    int lineCount = 0;
    const char* lineStart = start;
    
    while (*p && lineCount < 3) {
        if (*p == '\n' || *p == '\0') {
            int len = p - lineStart;
            if (len > CONTEXT_SIZE) len = CONTEXT_SIZE;
            
            // Añadir número de línea
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, 
                          "%4d | %.*s\n", targetLine + lineCount, len, lineStart);
            
            if (*p) lineStart = p + 1;
            lineCount++;
        }
        if (*p) p++;
    }
    
    e->context = strdup(buffer);
    e->errorPosition = (e->column - 1);
}

// Report an error (store error info and log it)
void error_report(const char* file, int line, int col, const char* msg, ErrorType type) {
    if (errorCount >= MAX_ERRORS) return;
    ErrorInfo* e = &errors[errorCount++];
    e->file = file;
    e->line = line;
    e->column = col;
    e->message = strdup(msg);
    e->type = type;
    extract_context(e);
    logger_log(LOG_ERROR, "[%s:%d:%d] %s", file, line, col, msg);
    
    // Añadir sugerencia de corrección según el tipo de error
    switch(type) {
        case ERROR_SYNTAX:
            logger_log(LOG_INFO, "Suggestion: Check syntax near this location");
            break;
        case ERROR_SEMANTIC:
            logger_log(LOG_INFO, "Suggestion: Verify variable declarations and scope");
            break;
        case ERROR_TYPE:
            logger_log(LOG_INFO, "Suggestion: Check type compatibility");
            break;
        // ...otros casos...
    }
}

// Set the source for context extraction
void error_set_source(const char* src) {
    sourceCode = src;
}

// Print the last error in minimal perfect style
void error_print_current(void) {
    if (errorCount == 0) return;
    ErrorInfo* e = &errors[errorCount - 1];

    // Header: icon, file, line, column y mensaje
    fprintf(stderr, COLOR_RED "❌ %s:%d:%d" COLOR_RESET " %s\n", e->file, e->line, e->column, e->message);

    // Context + caret
    if (e->context) {
        fprintf(stderr, "    %s\n", e->context);
        fprintf(stderr, "    %*s" COLOR_YELLOW "^\n" COLOR_RESET, e->errorPosition + 4, "");
    }
    
    // Stack trace (short)
    print_stack_short();
}

// Nuevo: Push debug info
void error_push_debug(const char* func, const char* file, int line, void* addr) {
    if (debugStackDepth >= STACK_MAX_DEPTH) return;
    debugStack[debugStackDepth].function = func;
    debugStack[debugStackDepth].file = file;
    debugStack[debugStackDepth].line = line;
    debugStack[debugStackDepth].address = addr;
    debugStackDepth++;
}

int error_get_count(void) {
    return errorCount;
}

const ErrorInfo* error_get_last(void) {
    return errorCount > 0 ? &errors[errorCount - 1] : NULL;
}

const char* get_error_message(ErrorType type) {
    switch(type) {
        case ERROR_SYNTAX:    return "Syntax Error";
        case ERROR_SEMANTIC:  return "Semantic Error";
        case ERROR_TYPE:      return "Type Error";
        case ERROR_MEMORY:    return "Memory Error";
        case ERROR_IO:        return "I/O Error";
        case ERROR_UNDEFINED: return "Undefined Symbol";
        case ERROR_RUNTIME:   return "Runtime Error";
        default:              return "Unknown Error";
    }
}
