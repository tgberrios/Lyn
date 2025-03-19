#ifndef LYN_LOGGER_H
#define LYN_LOGGER_H

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

void logger_init(const char* output_file);
void logger_set_level(LogLevel level);
void logger_log(LogLevel level, const char* format, ...);
void logger_close(void);

// Función para obtener el nivel de log actual
LogLevel logger_get_level(void);

#endif // LYN_LOGGER_H
