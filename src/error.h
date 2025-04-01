/**
 * @file error.h
 * @brief Header file for the error handling system in the Lyn compiler
 * 
 * This header defines the error handling system's interface, including error types,
 * error information structures, and functions for error reporting and management.
 */

#ifndef LYN_ERROR_H
#define LYN_ERROR_H

#include <stddef.h>

/**
 * @brief Enumeration of possible error types in the compiler
 */
typedef enum {
    ERROR_NONE = 0,      ///< No error occurred
    ERROR_SYNTAX,        ///< Syntax error in source code
    ERROR_SEMANTIC,      ///< Semantic error in program logic
    ERROR_TYPE,          ///< Type mismatch or invalid type usage
    ERROR_NAME,          ///< Invalid or undefined identifier
    ERROR_MEMORY,        ///< Memory allocation or management error
    ERROR_IO,            ///< Input/Output operation error
    ERROR_LIMIT,         ///< Resource limit exceeded
    ERROR_UNDEFINED,     ///< Undefined behavior or symbol
    ERROR_RUNTIME,       ///< Runtime execution error
    ERROR_MAX            ///< Maximum error type value (for bounds checking)
} ErrorType;

/**
 * @brief Structure containing detailed information about an error
 */
typedef struct {
    int line;            ///< Line number where the error occurred
    int column;          ///< Column number where the error occurred
    const char* file;    ///< Source file where the error occurred
    char* message;       ///< Human-readable error message
    char* context;       ///< Source code context around the error
    int contextLength;   ///< Length of the context string
    int errorPosition;   ///< Position of error within the context
    ErrorType type;      ///< Type of error that occurred
} ErrorInfo;

/**
 * @brief Reports an error with location and context information
 * 
 * Stores the error information and generates appropriate context from the source code.
 * The error will be available for retrieval via error_get_last() and can be displayed
 * using error_print_current().
 * 
 * @param module Name of the module where the error occurred
 * @param line Line number in the source file
 * @param col Column number in the source file
 * @param message Human-readable error message
 * @param type Type of error that occurred
 */
void error_report(const char* module, int line, int col, const char* message, ErrorType type);

/**
 * @brief Sets the source code for context extraction
 * 
 * This function must be called before error_report() to enable the extraction
 * of source code context around error locations.
 * 
 * @param source Pointer to the source code string
 */
void error_set_source(const char* source);

/**
 * @brief Prints the most recent error with context and stack trace
 * 
 * Displays the error information in a user-friendly format, including:
 * - Error location (file, line, column)
 * - Error message
 * - Source code context with caret pointing to the error
 * - Stack trace (if available)
 */
void error_print_current(void);

/**
 * @brief Gets the total number of errors reported
 * 
 * @return int Number of errors that have been reported
 */
int error_get_count(void);

/**
 * @brief Gets the most recently reported error
 * 
 * @return const ErrorInfo* Pointer to the last error, or NULL if no errors
 */
const ErrorInfo* error_get_last(void);

/**
 * @brief Gets a human-readable message for an error type
 * 
 * @param type The type of error
 * @return const char* String describing the error type
 */
const char* get_error_message(ErrorType type);

/**
 * @brief Pushes debug information onto the debug stack for stack trace generation
 * 
 * This function is used internally to track function calls for generating
 * detailed stack traces when errors occur.
 * 
 * @param func Name of the function
 * @param file Source file name
 * @param line Line number in source file
 * @param addr Function's memory address
 */
void error_push_debug(const char* func, const char* file, int line, void* addr);

#endif /* LYN_ERROR_H */
