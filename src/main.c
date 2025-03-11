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
    snprintf(command, sizeof(command), "gcc -o %s %s -lm", executablePath, outputPath);
    return system(command);
}

int main(int argc, char* argv[]) {
    // Inicializar el logger
    logger_init("lyn_compiler.log");
    logger_set_level(LOG_DEBUG);
    logger_log(LOG_INFO, "Iniciando compilación de %s", argv[1]);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    // Obtener el nombre base del archivo
    char* sourcePath = argv[1];
    char* baseName = strdup(sourcePath);
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = '\0';

    // Crear nombres de archivos
    char outputPath[256];
    char executablePath[256];
    snprintf(outputPath, sizeof(outputPath), "%s.c", baseName);
    snprintf(executablePath, sizeof(executablePath), "%s.out", baseName);

    // Leer archivo fuente
    char* source = readFile(sourcePath);
    
    // Inicializar lexer
    lexerInit(source);

    // Inicializar optimizador
    optimizer_init(OPT_LEVEL_2);

    // Parsear el código
    AstNode* ast = parseProgram();
    if (!ast) {
        fprintf(stderr, "Parsing failed\n");
        free(source);
        free(baseName);
        return 1;
    }

    // Registrar eventos importantes
    logger_log(LOG_DEBUG, "Archivo fuente leído: %d bytes", strlen(source));
    logger_log(LOG_INFO, "Compilando a C: %s", outputPath);

    // Optimizando AST
    logger_log(LOG_INFO, "Optimizando AST...");
    ast = optimize_ast(ast);

    // Compilar a C
    if (!compileToC(ast, outputPath)) {
        fprintf(stderr, "Compilation to C failed\n");
        freeAst(ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Compilar el archivo C generado
    printf("Compiling %s to %s...\n", outputPath, executablePath);
    if (compileOutputC(outputPath, executablePath) != 0) {
        fprintf(stderr, "C compilation failed\n");
        freeAst(ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Ejecutar el programa compilado
    printf("Running %s:\n", executablePath);
    printf("----------------------------------------\n");
    char runCommand[1024];
    snprintf(runCommand, sizeof(runCommand), "./%s", executablePath);
    system(runCommand);
    printf("----------------------------------------\n");

    // Liberar memoria
    freeAst(ast);
    free(source);
    free(baseName);

    logger_log(LOG_INFO, "Compilación completada exitosamente");
    logger_close();
    
    return 0;
}