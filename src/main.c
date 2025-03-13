#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "optimizer.h"  // Añadido: incluir optimizer.h
#include "logger.h"
#include <unistd.h>

char* readFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
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
    // Añadimos explícitamente -lm para garantizar enlace con la biblioteca matemática (sqrt)
    snprintf(command, sizeof(command), "gcc -o %s %s -lm -Wall", executablePath, outputPath);
    return system(command);
}

int main(int argc, char* argv[]) {
    // Inicializar el logger
    logger_init("lyn_compiler.log");
    logger_set_level(LOG_DEBUG);
    
    if (argc != 2) {
        fprintf(stderr, "Error: Incorrect usage\n");
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    logger_log(LOG_INFO, "Iniciando compilación de %s", argv[1]);

    // Obtener el nombre base del archivo
    char* sourcePath = argv[1];
    char* baseName = strdup(sourcePath);
    if (!baseName) {
        logger_log(LOG_ERROR, "Error: Failed to allocate memory for basename");
        return 1;
    }
    
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = '\0';

    // Crear nombres de archivos con manejo de errores
    char outputPath[256];
    char executablePath[256];
    snprintf(outputPath, sizeof(outputPath), "%s.c", baseName);
    snprintf(executablePath, sizeof(executablePath), "%s.out", baseName);

    // Leer archivo fuente con manejo de errores mejorado
    char* source = readFile(sourcePath);
    if (!source) {
        logger_log(LOG_ERROR, "Error: Failed to read source file");
        free(baseName);
        return 1;
    }
    
    // Inicializar lexer
    lexerInit(source);

    // Inicializar optimizador
    optimizer_init(OPT_LEVEL_2);

    // Parsear el código
    logger_log(LOG_INFO, "Parseando código fuente...");
    AstNode* ast = parseProgram();
    if (!ast) {
        logger_log(LOG_ERROR, "Error: Parsing failed");
        free(source);
        free(baseName);
        return 1;
    }

    // Registrar eventos importantes
    logger_log(LOG_DEBUG, "Archivo fuente leído: %d bytes", strlen(source));
    logger_log(LOG_INFO, "Compilando a C: %s", outputPath);

    // Optimizando AST - asegurarse de limpiar auto-asignaciones
    logger_log(LOG_INFO, "Optimizando AST...");
    ast = optimize_ast(ast);

    // Compilar a C
    logger_log(LOG_INFO, "Generando código C...");
    if (!compileToC(ast, outputPath)) {
        logger_log(LOG_ERROR, "Error: Compilation to C failed");
        freeAst(ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Compilar el archivo C generado
    logger_log(LOG_INFO, "Compilando código C a ejecutable...");
    printf("Compiling %s to %s...\n", outputPath, executablePath);
    if (compileOutputC(outputPath, executablePath) != 0) {
        logger_log(LOG_ERROR, "Error: C compilation failed");
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
        logger_log(LOG_ERROR, "Error during program execution. Return code: %d", result);
    } else {
        logger_log(LOG_INFO, "Program executed successfully");
    }

    // Limpieza segura
    if (ast) {
        logger_log(LOG_DEBUG, "Limpiando AST...");
        freeAst(ast);
        ast = NULL;
    }
    
    free(source);
    free(baseName);

    logger_log(LOG_INFO, "Compilación completada exitosamente");
    logger_close();
    
    printf("Compilation and execution successful!\n");
    
    return 0;
}