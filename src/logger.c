#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

static FILE* logFile = NULL;
static LogLevel currentLevel = LOG_INFO;
static const char* levelNames[] = {"DEBUG", "INFO", "WARNING", "ERROR"};

void logger_init(const char* filename) {
    logFile = fopen(filename, "a");
    if (!logFile) {
        fprintf(stderr, "Error: No se pudo abrir el archivo de log: %s\n", filename);
        return;
    }
    
    time_t now = time(NULL);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(logFile, "\n\n========== SESIÓN DE LOG INICIADA %s ==========\n\n", timeStr);
    fflush(logFile);
}

void logger_close(void) {
    if (logFile) {
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fprintf(logFile, "\n========== SESIÓN DE LOG FINALIZADA %s ==========\n", timeStr);
        fflush(logFile);
        fclose(logFile);
        logFile = NULL;
    }
}

void logger_set_level(LogLevel level) {
    currentLevel = level;
}

LogLevel logger_get_level(void) {
    return currentLevel;
}

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
    
    // Para mensajes de error, imprimir también en stderr
    if (level == LOG_ERROR) {
        fprintf(stderr, "[ERROR] ");
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}
