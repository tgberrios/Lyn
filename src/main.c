#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "optimizer.h"
#include "logger.h"
#include "error.h"
#include <unistd.h>

char* readFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file: %s\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        exit(1);
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

int compileOutputC(const char* outputPath, const char* executablePath) {
    char command[1024];
    // Añadimos explícitamente -lm para garantizar el enlace con la biblioteca matemática
    snprintf(command, sizeof(command), "gcc -o %s %s -lm -Wall", executablePath, outputPath);
    return system(command);
}

int main(int argc, char* argv[]) {
    // Inicializar el logger
    logger_init("lyn_compiler.log");
    logger_set_level(LOG_DEBUG);
    
    if (argc != 2) {
        fprintf(stderr, "Error: Uso incorrecto\n");
        fprintf(stderr, "Uso: %s <archivo_fuente>\n", argv[0]);
        return 1;
    }

    logger_log(LOG_INFO, "Iniciando compilación de %s", argv[1]);

    // Obtener el nombre base del archivo fuente
    char* sourcePath = argv[1];
    char* baseName = strdup(sourcePath);
    if (!baseName) {
        logger_log(LOG_ERROR, "Error: Falla al asignar memoria para basename");
        return 1;
    }
    
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = '\0';

    // Crear nombres de archivos para el código C generado y el ejecutable
    char outputPath[256];
    char executablePath[256];
    snprintf(outputPath, sizeof(outputPath), "%s.c", baseName);
    snprintf(executablePath, sizeof(executablePath), "%s.out", baseName);

    // Leer archivo fuente con manejo de errores mejorado
    char* source = readFile(sourcePath);
    if (!source) {
        logger_log(LOG_ERROR, "Error: Falla al leer el archivo fuente");
        free(baseName);
        return 1;
    }
    
    // Establecer el código fuente en el sistema de errores para extraer contexto
    error_set_source(source);

    // Inicializar lexer
    lexerInit(source);

    // Inicializar optimizador
    optimizer_init(OPT_LEVEL_2);

    // Parsear el código fuente
    logger_log(LOG_INFO, "Parseando código fuente...");
    AstNode* ast = parseProgram();
    if (!ast) {
        logger_log(LOG_ERROR, "Error: Falló el análisis sintáctico");
        error_report(argv[1], 0, 0, "Parsing failed", ERROR_SYNTAX);
        error_print_current();
        free(source);
        free(baseName);
        return 1;
    }

    // Registrar eventos importantes
    logger_log(LOG_DEBUG, "Archivo fuente leído: %d bytes", (int)strlen(source));
    logger_log(LOG_INFO, "Compilando a C: %s", outputPath);

    // Optimizar AST
    logger_log(LOG_INFO, "Optimizando AST...");
    ast = optimize_ast(ast);

    // Generar código C
    logger_log(LOG_INFO, "Generando código C...");
    if (!compileToC(ast, outputPath)) {
        logger_log(LOG_ERROR, "Error: Falló la compilación a C");
        error_report(argv[1], 0, 0, "Compilation to C failed", ERROR_RUNTIME);
        error_print_current();
        freeAst(ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Compilar el archivo C generado a un ejecutable
    logger_log(LOG_INFO, "Compilando código C a ejecutable...");
    printf("Compiling %s to %s...\n", outputPath, executablePath);
    if (compileOutputC(outputPath, executablePath) != 0) {
        logger_log(LOG_ERROR, "Error: Falló la compilación del código C");
        error_report(argv[1], 0, 0, "C compilation failed", ERROR_RUNTIME);
        error_print_current();
        freeAst(ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Ejecutar el programa compilado
    logger_log(LOG_INFO, "Ejecutando programa compilado...");
    printf("Running %s:\n", executablePath);
    printf("----------------------------------------\n");
    char runCommand[1024];
    snprintf(runCommand, sizeof(runCommand), "./%s", executablePath);
    int result = system(runCommand);
    printf("----------------------------------------\n");
    
    if (result != 0) {
        logger_log(LOG_ERROR, "Error durante la ejecución del programa. Código de retorno: %d", result);
        error_report(argv[1], 0, 0, "Program execution failed", ERROR_RUNTIME);
        error_print_current();
    } else {
        logger_log(LOG_INFO, "Programa ejecutado exitosamente");
    }

    // Limpieza segura
    if (ast) {
        logger_log(LOG_DEBUG, "Liberando AST...");
        freeAst(ast);
        ast = NULL;
    }
    
    free(source);
    free(baseName);

    logger_log(LOG_INFO, "Compilación completada exitosamente");
    logger_close();
    
    printf("¡Compilación y ejecución exitosas!\n");
    
    return 0;
}
