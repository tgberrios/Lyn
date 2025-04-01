/**
 * @file logger.c
 * @brief Implementation of the logging system for the Lyn compiler
 * 
 * This file implements a flexible logging system that supports different log levels,
 * file-based logging, and timestamp-based log entries. It provides functionality
 * for initializing, configuring, and writing log messages.
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

static FILE* logFile = NULL;                    ///< File handle for log output
static LogLevel currentLevel = LOG_INFO;        ///< Current logging level threshold
static const char* levelNames[] = {             ///< String names for log levels
    "DEBUG",    ///< Debug level messages
    "INFO",     ///< Information level messages
    "WARNING",  ///< Warning level messages
    "ERROR"     ///< Error level messages
};

/**
 * @brief Initializes the logging system with a specified log file
 * 
 * Opens a log file in append mode and writes a session start marker
 * with the current timestamp.
 * 
 * @param filename Path to the log file to create or append to
 */
void logger_init(const char* filename) {
    logFile = fopen(filename, "a");
    if (!logFile) {
        fprintf(stderr, "Error: Could not open log file: %s\n", filename);
        return;
    }
    
    time_t now = time(NULL);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(logFile, "\n\n========== LOG SESSION STARTED %s ==========\n\n", timeStr);
    fflush(logFile);
}

/**
 * @brief Closes the logging system and writes a session end marker
 * 
 * Writes a session end marker with the current timestamp and closes
 * the log file. This function should be called before program termination
 * to ensure all log messages are properly written.
 */
void logger_close(void) {
    if (logFile) {
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fprintf(logFile, "\n========== LOG SESSION ENDED %s ==========\n", timeStr);
        fflush(logFile);
        fclose(logFile);
        logFile = NULL;
    }
}

/**
 * @brief Sets the current logging level threshold
 * 
 * Messages with a level lower than the threshold will not be logged.
 * 
 * @param level The new logging level threshold
 */
void logger_set_level(LogLevel level) {
    currentLevel = level;
}

/**
 * @brief Gets the current logging level threshold
 * 
 * @return LogLevel The current logging level threshold
 */
LogLevel logger_get_level(void) {
    return currentLevel;
}

/**
 * @brief Logs a message with the specified level and format
 * 
 * Writes a formatted message to the log file with timestamp and level information.
 * Error messages are also written to stderr for immediate visibility.
 * 
 * @param level The level of the log message
 * @param format Format string for the message
 * @param ... Variable arguments for the format string
 */
void logger_log(LogLevel level, const char* format, ...) {
    if (level < currentLevel || !logFile) return;
    
    time_t now = time(NULL);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(logFile, "[%s] [%s] ", timeStr, levelNames[level]);
    
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);
    
    fprintf(logFile, "\n");
    fflush(logFile);
    
    // For error messages, also print to stderr
    if (level == LOG_ERROR) {
        fprintf(stderr, "[ERROR] ");
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}
