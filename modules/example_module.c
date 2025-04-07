/**
 * @file example_module.c
 * @brief Módulo dinámico de ejemplo para el sistema de módulos de Lyn
 * 
 * Este módulo implementa algunas funciones matemáticas básicas que pueden
 * ser cargadas dinámicamente por el sistema de módulos de Lyn.
 * 
 * Compilación:
 * gcc -shared -fPIC -o example_module.so example_module.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Definición de estructuras (compatibles con el sistema de módulos)
typedef struct {
    int major;
    int minor;
    int patch;
} ModuleVersion;

typedef struct {
    ModuleVersion version;
    const char* author;
    const char* description;
    const char* license;
} ModuleInfo;

typedef struct {
    char name[256];
    int visibility;
    void* symbol;
    char type[64];
} ExportDefinition;

// Definición de funciones exportadas

/**
 * @brief Suma dos números
 */
double add(double a, double b) {
    printf("Usando la función 'add' del módulo dinámico\n");
    return a + b;
}

/**
 * @brief Resta dos números
 */
double subtract(double a, double b) {
    printf("Usando la función 'subtract' del módulo dinámico\n");
    return a - b;
}

/**
 * @brief Multiplica dos números
 */
double multiply(double a, double b) {
    printf("Usando la función 'multiply' del módulo dinámico\n");
    return a * b;
}

/**
 * @brief Divide dos números
 */
double divide(double a, double b) {
    printf("Usando la función 'divide' del módulo dinámico\n");
    if (b == 0) {
        fprintf(stderr, "Error: División por cero (desde módulo dinámico)\n");
        return 0;
    }
    return a / b;
}

/**
 * @brief Potencia de un número
 */
double power(double base, double exponent) {
    printf("Usando la función 'power' del módulo dinámico\n");
    return pow(base, exponent);
}

/**
 * @brief Formato de cadenas
 */
double format(const char* template, const char* arg) {
    printf("Usando la función 'format' del módulo dinámico\n");
    printf("%s %s\n", template, arg);
    return 1.0;
}

/**
 * @brief Devuelve información sobre el módulo
 */
ModuleInfo* getModuleInfo() {
    static ModuleInfo info = {
        {1, 0, 0},           // version
        "Claude",            // author
        "Módulo de ejemplo para demostrar la carga dinámica", // description
        "MIT"                // license
    };
    return &info;
}

/**
 * @brief Devuelve las exportaciones del módulo
 */
ExportDefinition* getExports(int* count) {
    static ExportDefinition exports[] = {
        {"add", 1, (void*)add, "double(double,double)"},
        {"subtract", 1, (void*)subtract, "double(double,double)"},
        {"multiply", 1, (void*)multiply, "double(double,double)"},
        {"divide", 1, (void*)divide, "double(double,double)"},
        {"power", 1, (void*)power, "double(double,double)"},
        {"format", 1, (void*)format, "double(const char*,const char*)"}
    };
    
    *count = sizeof(exports) / sizeof(exports[0]);
    return exports;
} 