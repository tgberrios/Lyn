/**
 * @file error.c
 * @brief Implementation of error handling and reporting system for the Lyn compiler
 * 
 * This file provides a comprehensive error handling system that includes
 * error reporting, stack trace generation, source code context extraction,
 * and debug information tracking.
 */

#define _GNU_SOURCE         /* Enable Dl_info in dlfcn.h */
#include <dlfcn.h>         /* Must be first to make Dl_info available */
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

// ANSI color codes for terminal output
#define COLOR_RESET     "\x1b[0m"
#define COLOR_RED       "\x1b[31m"
#define COLOR_CYAN      "\x1b[36m"
#define COLOR_YELLOW    "\x1b[33m"

/**
 * @brief Structure to store debugging information for stack traces
 */
typedef struct {
    const char* function;  ///< Name of the function
    const char* file;      ///< Source file name
    int line;             ///< Line number in source file
    void* address;        ///< Memory address of the function
} DebugInfo;

static ErrorInfo errors[MAX_ERRORS];
static int errorCount = 0;
static const char* sourceCode = NULL;

// Debug stack for tracking function calls
static DebugInfo debugStack[STACK_MAX_DEPTH];
static int debugStackDepth = 0;

/**
 * @brief Prints a short stack trace (maximum 3 frames)
 * 
 * Uses backtrace and dladdr to generate a human-readable stack trace
 * with function names and source locations when available.
 */
static void print_stack_short(void) {
    void* buffer[STACK_MAX_DEPTH];
    int frames = backtrace(buffer, STACK_MAX_DEPTH);
    char** symbols = backtrace_symbols(buffer, frames);
    
    fprintf(stderr, COLOR_CYAN "Stack Trace:" COLOR_RESET "\n");
    
    for (int i = 1; i < frames && i < 5; i++) {
        Dl_info info;
        if (dladdr(buffer[i], &info) && info.dli_sname) {
            // Find corresponding debug info
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

/**
 * @brief Extracts source code context around an error location
 * 
 * Retrieves up to 3 lines of source code context around the error location,
 * including line numbers and proper formatting.
 * 
 * @param e Pointer to the ErrorInfo structure to populate with context
 */
static void extract_context(ErrorInfo* e) {
    if (!sourceCode) return;
    
    // Find the start of the previous line
    const char* start = sourceCode;
    const char* p = sourceCode;
    int ln = 1;
    int targetLine = e->line > 1 ? e->line - 1 : e->line;
    
    while (*p && ln < targetLine) {
        if (*p == '\n') { ln++; start = p + 1; }
        p++;
    }

    // Capture 3 lines of context
    char buffer[CONTEXT_SIZE * 3 + 1] = {0};
    int pos = 0;
    int lineCount = 0;
    const char* lineStart = start;
    
    while (*p && lineCount < 3) {
        if (*p == '\n' || *p == '\0') {
            int len = p - lineStart;
            if (len > CONTEXT_SIZE) len = CONTEXT_SIZE;
            
            // Add line number
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

/**
 * @brief Reports an error with source location and context
 * 
 * Stores error information and logs it with appropriate context and suggestions.
 * 
 * @param file Source file name where the error occurred
 * @param line Line number in the source file
 * @param col Column number in the source file
 * @param msg Error message
 * @param type Type of error that occurred
 */
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
    
    // Add correction suggestions based on error type
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
        // ...other cases...
    }
}

/**
 * @brief Sets the source code for context extraction
 * 
 * @param src Pointer to the source code string
 */
void error_set_source(const char* src) {
    sourceCode = src;
}

/**
 * @brief Prints the last reported error with context and stack trace
 * 
 * Displays error information in a user-friendly format including:
 * - Error location and message
 * - Source code context with caret pointing to error
 * - Short stack trace
 */
void error_print_current(void) {
    if (errorCount == 0) return;
    ErrorInfo* e = &errors[errorCount - 1];

    // Header: icon, file, line, column and message
    fprintf(stderr, COLOR_RED "❌ %s:%d:%d" COLOR_RESET " %s\n", e->file, e->line, e->column, e->message);

    // Context + caret
    if (e->context) {
        fprintf(stderr, "    %s\n", e->context);
        fprintf(stderr, "    %*s" COLOR_YELLOW "^\n" COLOR_RESET, e->errorPosition + 4, "");
    }
    
    // Stack trace (short)
    print_stack_short();
}

/**
 * @brief Pushes debug information onto the debug stack
 * 
 * @param func Function name
 * @param file Source file name
 * @param line Line number
 * @param addr Function address
 */
void error_push_debug(const char* func, const char* file, int line, void* addr) {
    if (debugStackDepth >= STACK_MAX_DEPTH) return;
    debugStack[debugStackDepth].function = func;
    debugStack[debugStackDepth].file = file;
    debugStack[debugStackDepth].line = line;
    debugStack[debugStackDepth].address = addr;
    debugStackDepth++;
}

/**
 * @brief Gets the total number of errors reported
 * 
 * @return int Number of errors reported
 */
int error_get_count(void) {
    return errorCount;
}

/**
 * @brief Gets the most recently reported error
 * 
 * @return const ErrorInfo* Pointer to the last error, or NULL if no errors
 */
const ErrorInfo* error_get_last(void) {
    return errorCount > 0 ? &errors[errorCount - 1] : NULL;
}

/**
 * @brief Gets a human-readable message for an error type
 * 
 * @param type The type of error
 * @return const char* String describing the error type
 */
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
