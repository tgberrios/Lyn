/**
 * @file logger.h
 * @brief Header file for the logging system in the Lyn compiler
 * 
 * This header defines the interface for the logging system, including log levels
 * and functions for logging configuration and message output. The logging system
 * supports different severity levels and file-based logging with timestamps.
 */

#ifndef LYN_LOGGER_H
#define LYN_LOGGER_H

/**
 * @brief Enumeration of available logging levels
 * 
 * Defines the different severity levels for log messages, ordered from
 * most verbose (DEBUG) to most critical (ERROR).
 */
typedef enum {
    LOG_DEBUG,   ///< Debug level - detailed information for debugging
    LOG_INFO,    ///< Info level - general information about program execution
    LOG_WARNING, ///< Warning level - potential issues that don't stop execution
    LOG_ERROR    ///< Error level - serious problems that may affect functionality
} LogLevel;

/**
 * @brief Initializes the logging system with a specified output file
 * 
 * Opens a log file in append mode and writes a session start marker.
 * This function must be called before using any other logging functions.
 * 
 * @param output_file Path to the file where logs will be written
 */
void logger_init(const char* output_file);

/**
 * @brief Sets the current logging level threshold
 * 
 * Messages with a level lower than the threshold will not be logged.
 * This allows for runtime control of logging verbosity.
 * 
 * @param level The new logging level threshold
 */
void logger_set_level(LogLevel level);

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
void logger_log(LogLevel level, const char* format, ...);

/**
 * @brief Closes the logging system and writes a session end marker
 * 
 * Writes a session end marker with the current timestamp and closes
 * the log file. This function should be called before program termination
 * to ensure all log messages are properly written.
 */
void logger_close(void);

/**
 * @brief Gets the current logging level threshold
 * 
 * @return LogLevel The current logging level threshold
 */
LogLevel logger_get_level(void);

#endif // LYN_LOGGER_H
