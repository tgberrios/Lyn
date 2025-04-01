# Documentación del Proyecto Lyn

## Índice

1. [Sistema de Compilación](#sistema-de-compilación)

   - [Arquitectura](#arquitectura)
   - [Fases de Compilación](#fases-de-compilación)
   - [Componentes](#componentes)

2. [Sistema de Análisis Léxico](#sistema-de-análisis-léxico)

   - [Descripción General](#descripción-general)
   - [Tipos de Tokens](#tipos-de-tokens)
   - [Funcionalidades](#funcionalidades)

3. [Sistema de Análisis Sintáctico](#sistema-de-análisis-sintáctico)

   - [Descripción General](#descripción-general-1)
   - [Estructuras de Control](#estructuras-de-control)
   - [Expresiones](#expresiones)

4. [Sistema de Tipos](#sistema-de-tipos)

   - [Descripción General](#descripción-general)
   - [Tipos Básicos](#tipos-básicos)
   - [Tipos Compuestos](#tipos-compuestos)
   - [Inferencia de Tipos](#inferencia-de-tipos)

5. [Sistema de AST](#sistema-de-ast)

   - [Descripción General](#descripción-general-2)
   - [Tipos de Nodos](#tipos-de-nodos-2)
   - [Gestión de Memoria](#gestión-de-memoria-2)

6. [Sistema de Optimización](#sistema-de-optimización)

   - [Niveles de Optimización](#niveles-de-optimización)
   - [Optimizaciones Implementadas](#optimizaciones-implementadas)
   - [Configuración](#configuración)

7. [Sistema de Memoria](#sistema-de-memoria)

   - [Gestión de Memoria](#gestión-de-memoria-1)
   - [Memory Pooling](#memory-pooling)
   - [Garbage Collection](#garbage-collection)

8. [Sistema de Errores](#sistema-de-errores)

   - [Tipos de Errores](#tipos-de-errores)
   - [Manejo de Errores](#manejo-de-errores)
   - [Reporte de Errores](#reporte-de-errores)

9. [Sistema de Logging](#sistema-de-logging)

   - [Descripción General](#descripción-general)
   - [Niveles de Log](#niveles-de-log)
   - [Funcionalidades](#funcionalidades)
   - [Configuración](#configuración)

10. [Sistema de Módulos](#sistema-de-módulos)

    - [Descripción General](#descripción-general)
    - [Estructuras Principales](#estructuras-principales)
    - [Funcionalidades](#funcionalidades)

11. [Sistema de Aspectos](#sistema-de-aspectos)

    - [Aspectos](#aspectos)
    - [Pointcuts](#pointcuts)
    - [Advice](#advice)

12. [Sistema de Macros](#sistema-de-macros)

    - [Evaluación de Macros](#evaluación-de-macros)
    - [Expansión de Macros](#expansión-de-macros)
    - [Configuración](#configuración-2)

13. [Sistema de Reflexión](#sistema-de-reflexión)

    - [Descripción General](#descripción-general)
    - [Estructura de Tipos en Tiempo de Ejecución](#estructura-de-tipos-en-tiempo-de-ejecución)
    - [Funcionalidades](#funcionalidades)

14. [Sistema de Tabla de Símbolos](#sistema-de-tabla-de-símbolos)

    - [Descripción General](#descripción-general)
    - [Estructuras Principales](#estructuras-principales)
    - [Funcionalidades](#funcionalidades)

15. [Sistema de Templates](#sistema-de-templates)
    - [Plantillas](#plantillas)
    - [Instanciación](#instanciación)
    - [Configuración](#configuración-3)

## Sistema de Compilación

### Descripción General

El sistema de compilación es el núcleo del proyecto, responsable de transformar el código fuente en código ejecutable. Implementa un compilador que traduce el AST a código C, incluyendo verificación de tipos y seguimiento de estadísticas.

### Estadísticas del Compilador

El sistema mantiene estadísticas detalladas del proceso de compilación:

```c
typedef struct {
    int nodes_processed;        // Número de nodos AST procesados
    int functions_compiled;     // Número de funciones compiladas
    int variables_declared;     // Número de variables declaradas
    int type_errors_detected;   // Número de errores de tipo detectados
} CompilerStats;
```

### Funcionalidades

1. **Gestión del Sistema**

   - `compiler_set_debug_level()`: Configuración de niveles de depuración
   - `compiler_get_stats()`: Obtención de estadísticas de compilación

2. **Compilación**

   - `compileToC()`: Compilación principal del AST a código C
   - Verificación de tipos en asignaciones y llamadas a funciones

3. **Verificación de Tipos**
   - `check_assignment_types()`: Verificación de tipos en asignaciones
   - `check_function_call_types()`: Verificación de tipos en llamadas a funciones

### Características

1. **Compilación**

   - Traducción de AST a código C
   - Generación de código optimizado
   - Manejo de errores y advertencias

2. **Monitoreo**

   - Seguimiento de nodos procesados
   - Conteo de funciones compiladas
   - Detección de errores de tipo
   - Estadísticas de compilación

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de errores
   - Sistema de logging

### Arquitectura

El compilador de Lyn está diseñado como un compilador de una pasada que traduce el código fuente de Lyn a C. La arquitectura está organizada en varios componentes principales:

1. **Frontend**

   - Analizador léxico (Lexer)
   - Analizador sintáctico (Parser)
   - Árbol de sintaxis abstracta (AST)

2. **Backend**

   - Generador de código C
   - Optimizador
   - Sistema de tipos

3. **Sistema de Gestión de Errores**
   - Detección de errores
   - Reporte de errores
   - Recuperación de errores

### Fases de Compilación

El proceso de compilación se divide en las siguientes fases:

1. **Análisis Léxico**

   - Tokenización del código fuente
   - Identificación de palabras clave y símbolos
   - Manejo de espacios en blanco y comentarios
   - Sistema de palabras clave extensible
   - Manejo de errores léxicos
   - Soporte para:
     - Comentarios de línea (//)
     - Comentarios de bloque (/\* \*/)
     - Espacios en blanco y saltos de línea
     - Identificadores y literales
     - Operadores y delimitadores

2. **Análisis Sintáctico**

   - Construcción del AST
   - Validación de la estructura del programa
   - Detección de errores de sintaxis
   - Manejo de expresiones y declaraciones
   - Soporte para características avanzadas:
     - Funciones lambda
     - Clases y objetos
     - Aspectos (AspectJ)
     - Pattern matching
     - Composición de funciones

3. **Análisis Semántico**

   - Verificación de tipos
   - Comprobación de declaraciones
   - Validación de expresiones

4. **Generación de Código**
   - Traducción a C
   - Optimización de código
   - Generación de código objeto

### Componentes

#### Sistema de Variables

- Tabla de variables con información de tipos
- Gestión de alcance de variables
- Sistema de declaración y uso
- Límite configurable de variables (MAX_VARIABLES)

#### Generador de Código

- Emisión de código C estructurado
- Manejo de indentación
- Generación de preámbulo con includes
- Constantes y definiciones

#### Sistema de Tipos

El sistema de tipos implementa un sistema de tipos estático con inferencia de tipos, soporte para tipos primitivos y compuestos, y capacidades de reflexión. El sistema está diseñado para ser flexible y extensible, permitiendo la definición de tipos personalizados y la verificación de tipos en tiempo de compilación.

### Tipos Básicos

El sistema implementa los siguientes tipos básicos:

```c
typedef enum {
    TYPE_INT,      // Enteros
    TYPE_FLOAT,    // Números de punto flotante
    TYPE_BOOL,     // Valores booleanos
    TYPE_STRING,   // Cadenas de texto
    TYPE_VOID,     // Tipo vacío
    TYPE_UNKNOWN,  // Tipo desconocido
    TYPE_ARRAY,    // Arrays
    TYPE_CLASS,    // Clases
    TYPE_FUNCTION, // Funciones
    TYPE_LAMBDA,   // Funciones lambda
    TYPE_CURRIED,  // Funciones curried
    TYPE_OBJECT,   // Objetos
    TYPE_NULL      // Valor nulo
} TypeKind;
```

### Tipos Compuestos

El sistema soporta varios tipos compuestos:

1. **Arrays**

   ```c
   struct {
       struct Type* elementType;
   } arrayType;
   ```

2. **Clases**

   ```c
   struct {
       char name[64];
       struct Type* baseClass;
   } classType;
   ```

3. **Funciones**

   ```c
   struct {
       struct Type* returnType;
       struct Type** paramTypes;
       int paramCount;
   } functionType;
   ```

4. **Funciones Curried**
   ```c
   struct {
       struct Type* baseType;
       int appliedArgCount;
   } curriedType;
   ```

### Reflexión

El sistema implementa capacidades de reflexión a través de las siguientes estructuras:

1. **Información de Campos**

   ```c
   typedef struct {
       char name[256];
       Type* type;
       int modifiers;
   } FieldInfo;
   ```

2. **Información de Métodos**

   ```c
   typedef struct {
       char name[256];
       Type* returnType;
       Type** paramTypes;
       int paramCount;
       char** paramNames;
       int modifiers;
   } MethodInfo;
   ```

3. **Información de Tipos**
   ```c
   typedef struct {
       char name[256];
       Type* type;
       FieldInfo** fields;
       int fieldCount;
       MethodInfo** methods;
       int methodCount;
       Type* baseType;
       bool isBuiltin;
   } TypeInfo;
   ```

### Funcionalidades

1. **Creación de Tipos**

   - `createBasicType`: Creación de tipos básicos
   - `createArrayType`: Creación de tipos de array
   - `createClassType`: Creación de tipos de clase
   - `createFunctionType`: Creación de tipos de función
   - `create_curried_type`: Creación de tipos curried

2. **Verificación de Tipos**

   - `are_types_compatible`: Verificación de compatibilidad
   - `is_subtype_of`: Verificación de subtipos
   - `are_types_equal`: Comparación de tipos
   - `check_ast_types`: Verificación de tipos en AST

3. **Inferencia de Tipos**

   - `infer_type`: Inferencia de tipos en nodos AST
   - `infer_type_from_literal`: Inferencia desde literales
   - `infer_type_from_binary_op`: Inferencia en operaciones binarias
   - `get_common_type`: Obtención de tipo común

4. **Introspección**
   - `get_type_info`: Obtención de información de tipo
   - `get_fields`: Obtención de campos de un tipo
   - `get_methods`: Obtención de métodos de un tipo
   - `typeof_value`: Obtención del tipo de una expresión

### Estadísticas y Depuración

El sistema incluye capacidades de monitoreo y depuración:

1. **Estadísticas**

   ```c
   typedef struct {
       int types_created;
       int types_freed;
       int type_errors_detected;
       int classes_declared;
       int functions_typed;
   } TypeSystemStats;
   ```

2. **Configuración**
   - `types_set_debug_level`: Configuración de nivel de depuración
   - `types_get_debug_level`: Obtención de nivel actual
   - `types_get_stats`: Obtención de estadísticas

### Integración

El sistema se integra con:

- Sistema de AST para inferencia y verificación
- Sistema de errores para reportes de tipo
- Sistema de logging para depuración
- Sistema de reflexión para introspección

## Sistema de Análisis Léxico

### Descripción General

El sistema de análisis léxico implementa un lexer robusto para la tokenización del código fuente con las siguientes características:

1. **Estructura de Token**

   ```c
   typedef struct {
       TokenType type;         // Tipo del token
       char lexeme[256];       // Cadena del token
       int line;               // Línea donde aparece
       int col;                // Columna donde aparece
       union {
           char string[256];   // Para valores string
           double number;      // Para valores numéricos
       } value;
   } Token;
   ```

2. **Estado del Lexer**
   ```c
   typedef struct {
       const char *source;     // Fuente de texto
       int position;           // Posición actual en la fuente
       int line;               // Línea actual
       int col;                // Columna actual
   } LexerState;
   ```

### Tipos de Tokens

El sistema implementa una amplia variedad de tokens:

1. **Tokens Básicos**

   - `TOKEN_EOF`: Fin de archivo
   - `TOKEN_IDENTIFIER`: Identificadores
   - `TOKEN_NUMBER`: Números
   - `TOKEN_STRING`: Cadenas de texto
   - `TOKEN_UNKNOWN`: Caracteres no reconocidos
   - `TOKEN_INVALID`: Token inválido

2. **Operadores**

   - Aritméticos: `TOKEN_PLUS`, `TOKEN_MINUS`, `TOKEN_ASTERISK`, `TOKEN_SLASH`
   - Asignación: `TOKEN_ASSIGN`
   - Comparación: `TOKEN_GT`, `TOKEN_LT`, `TOKEN_GTE`, `TOKEN_LTE`, `TOKEN_EQ`, `TOKEN_NEQ`
   - Flechas: `TOKEN_ARROW`, `TOKEN_FAT_ARROW`
   - Composición: `TOKEN_COMPOSE`
   - Macro: `TOKEN_CONCAT`, `TOKEN_STRINGIFY`

3. **Delimitadores**

   - Paréntesis: `TOKEN_LPAREN`, `TOKEN_RPAREN`
   - Corchetes: `TOKEN_LBRACKET`, `TOKEN_RBRACKET`
   - Llaves: `TOKEN_LBRACE`, `TOKEN_RBRACE`
   - Puntuación: `TOKEN_COMMA`, `TOKEN_DOT`, `TOKEN_DOTS`, `TOKEN_SEMICOLON`, `TOKEN_COLON`

4. **Palabras Clave**
   - Control: `TOKEN_IF`, `TOKEN_ELSE`, `TOKEN_FOR`, `TOKEN_WHILE`, `TOKEN_DO`, `TOKEN_SWITCH`, `TOKEN_CASE`, `TOKEN_DEFAULT`, `TOKEN_BREAK`
   - Funciones: `TOKEN_FUNC`, `TOKEN_RETURN`, `TOKEN_PRINT`
   - Clases: `TOKEN_CLASS`, `TOKEN_NEW`, `TOKEN_THIS`
   - Módulos: `TOKEN_MODULE`, `TOKEN_IMPORT`, `TOKEN_EXPORT`
   - Tipos: `TOKEN_INT`, `TOKEN_FLOAT`
   - Valores: `TOKEN_TRUE`, `TOKEN_FALSE`
   - Lógica: `TOKEN_AND`, `TOKEN_OR`
   - Excepciones: `TOKEN_TRY`, `TOKEN_CATCH`, `TOKEN_FINALLY`, `TOKEN_THROW`
   - Pattern Matching: `TOKEN_MATCH`, `TOKEN_WHEN`, `TOKEN_OTHERWISE`
   - Aspectos: `TOKEN_ASPECT`, `TOKEN_POINTCUT`, `TOKEN_ADVICE`, `TOKEN_BEFORE`, `TOKEN_AFTER`, `TOKEN_AROUND`
   - Macros: `TOKEN_MACRO`, `TOKEN_EXPAND`

### Funcionalidades

El sistema implementa las siguientes funcionalidades:

1. **Inicialización**

   - `lexerInit`: Inicialización con fuente de texto
   - `lexerInitialize`: Inicialización del sistema

2. **Tokenización**

   - `getNextToken`: Obtención del siguiente token
   - `tokenTypeToString`: Conversión de tipo de token a string

3. **Gestión de Estado**

   - `lexSaveState`: Guardado del estado actual
   - `lexRestoreState`: Restauración de estado anterior

4. **Configuración**
   - `lexer_set_debug_level`: Configuración de nivel de depuración

## Sistema de Análisis Sintáctico

### Descripción General

El sistema de análisis sintáctico implementa un parser que convierte el flujo de tokens en un Árbol de Sintaxis Abstracta (AST). El sistema está diseñado para ser robusto y proporcionar información detallada sobre el proceso de análisis.

### Funcionalidades Principales

1. **Análisis de Programa**

   - `parseProgram()`: Función principal que inicia el análisis del programa fuente
   - Genera un AST completo a partir del código fuente
   - Maneja la estructura general del programa

2. **Gestión de Estado**

   - `nextToken()`: Avanza al siguiente token en el flujo de entrada
   - `expectToken()`: Verifica y consume un token esperado
   - Manejo de errores cuando se encuentran tokens inesperados

3. **Construcción de AST**

   - `parseBlock()`: Analiza bloques de código
   - Generación de nodos AST para diferentes estructuras
   - Manejo de listas de nodos con conteo dinámico

4. **Gestión de Memoria**
   - `freeAst()`: Libera la memoria del AST generado
   - Manejo eficiente de recursos

### Características de Depuración

1. **Niveles de Depuración**

   - `parser_set_debug_level()`: Configura el nivel de detalle (0-3)
   - `parser_get_debug_level()`: Obtiene el nivel actual
   - Niveles de detalle configurables para diferentes necesidades

2. **Estadísticas**
   - `parser_get_stats()`: Proporciona métricas del proceso de análisis
   - Seguimiento de nodos creados
   - Monitoreo de errores encontrados

### Integración

El sistema se integra con:

- Sistema de AST para la representación del código
- Sistema de manejo de errores para reportes detallados
- Sistema de logging para seguimiento del proceso

## Sistema de Tipos

### Inferencia de Tipos

El sistema implementa un robusto sistema de inferencia de tipos con las siguientes características:

1. **Funciones de Inferencia**

   - `infer_type`: Inferencia general desde nodos AST
   - `infer_type_from_literal`: Inferencia desde literales
   - `infer_type_from_binary_op`: Inferencia desde operaciones binarias
   - `get_common_type`: Obtención del tipo común entre dos tipos

2. **Verificación de Tipos**

   - `check_ast_types`: Verificación de tipos en AST
   - `check_types`: Verificación contra tipo esperado
   - `are_types_compatible`: Compatibilidad entre tipos
   - `is_subtype_of`: Verificación de subtipado
   - `are_types_equal`: Igualdad de tipos

3. **Introspección de Tipos**

   - `get_type_info`: Información completa de un tipo
   - `get_fields`: Campos de un tipo
   - `get_methods`: Métodos de un tipo
   - `get_member_type`: Tipo de un miembro de clase

4. **Utilidades**

   - `typeToString`: Conversión a string
   - `typeToC`: Conversión a tipo C
   - `type_kind_to_string`: Conversión de tipo a string
   - `typeof_value`: Tipo de una expresión

5. **Gestión de Memoria**

   - `createBasicType`: Creación de tipos básicos
   - `createArrayType`: Creación de tipos array
   - `createClassType`: Creación de tipos clase
   - `createFunctionType`: Creación de tipos función
   - `create_curried_type`: Creación de tipos curried
   - `clone_type`: Clonación de tipos
   - `freeType`: Liberación de tipos

6. **Estadísticas y Depuración**

   ```c
   typedef struct {
       int types_created;
       int types_freed;
       int type_errors_detected;
       int classes_declared;
       int functions_typed;
   } TypeSystemStats;
   ```

7. **Configuración**
   - `types_set_debug_level`: Configuración de nivel de depuración
   - `types_get_debug_level`: Obtención de nivel de depuración
   - `types_get_stats`: Obtención de estadísticas

## Sistema de Optimización

### Descripción General

El sistema de optimización implementa diversas técnicas para mejorar el rendimiento del código generado. Opera sobre el AST y aplica transformaciones optimizadoras según el nivel configurado.

### Niveles de Optimización

```c
typedef enum {
    OPT_LEVEL_0 = 0,  // Sin optimización
    OPT_LEVEL_1 = 1,  // Optimizaciones básicas
    OPT_LEVEL_2 = 2   // Optimizaciones avanzadas
} OptimizerLevel;
```

### Estadísticas de Optimización

```c
typedef struct {
    int constant_folding_applied;      // Plegado de constantes aplicado
    int dead_code_removed;             // Código muerto eliminado
    int redundant_assignments_removed;  // Asignaciones redundantes eliminadas
    int constants_propagated;          // Constantes propagadas
    int cse_eliminated;                // Subexpresiones comunes eliminadas
    int variables_scoped;              // Variables con análisis de alcance
    int total_optimizations;           // Total de optimizaciones aplicadas
} OptimizationStats;
```

### Opciones de Optimización

```c
typedef struct {
    bool enable_constant_folding;           // Plegado de constantes
    bool enable_dead_code_elimination;      // Eliminación de código muerto
    bool enable_redundant_stmt_removal;     // Eliminación de sentencias redundantes
    bool enable_constant_propagation;       // Propagación de constantes
    bool enable_common_subexpr_elimination; // Eliminación de subexpresiones comunes
    bool enable_scope_analysis;             // Análisis de alcance
} OptimizerOptions;
```

### Funcionalidades

1. **Gestión del Sistema**

   - `optimizer_init()`: Inicialización con nivel de optimización
   - `optimizer_set_debug_level()`: Configuración de depuración
   - `optimizer_get_debug_level()`: Obtención del nivel de depuración

2. **Optimización**
   - `optimize_ast()`: Optimización principal del AST
   - `optimizer_get_stats()`: Obtención de estadísticas
   - `optimizer_set_options()`: Configuración de opciones
   - `optimizer_get_options()`: Obtención de opciones actuales

### Características

1. **Optimizaciones**

   - Plegado de constantes
   - Eliminación de código muerto
   - Eliminación de asignaciones redundantes
   - Propagación de constantes
   - Eliminación de subexpresiones comunes
   - Análisis de alcance de variables

2. **Monitoreo**

   - Seguimiento de optimizaciones aplicadas
   - Estadísticas detalladas
   - Niveles de depuración configurables

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de errores
   - Sistema de logging

## Sistema de Memoria

### Descripción General

El sistema de gestión de memoria implementa un conjunto de funcionalidades para el manejo eficiente de memoria, incluyendo asignación dinámica, pooling de memoria para objetos fijos, y un garbage collector opcional. El sistema está diseñado para ser robusto y proporcionar información detallada sobre el uso de memoria.

### Funcionalidades Básicas

1. **Inicialización y Limpieza**

   - `memory_init()`: Inicialización del subsistema
   - `memory_cleanup()`: Limpieza de recursos
   - `memory_stats()`: Visualización de estadísticas

2. **Asignación de Memoria**

   - `memory_alloc()`: Asignación de memoria dinámica
   - `memory_realloc()`: Reasignación de memoria
   - `memory_free()`: Liberación de memoria
   - `memory_strdup()`: Duplicación de cadenas

3. **Configuración**
   - `memory_set_debug_level()`: Configuración de nivel de depuración
   - `memory_get_debug_level()`: Obtención de nivel actual

### Memory Pooling

El sistema implementa un mecanismo de pooling para objetos de tamaño fijo:

1. **Estructura del Pool**

   ```c
   typedef struct MemoryPool MemoryPool;
   ```

2. **Funciones del Pool**

   - `memory_pool_create()`: Creación de pool con tamaño fijo
   - `memory_pool_alloc()`: Asignación desde el pool
   - `memory_pool_free()`: Liberación al pool
   - `memory_pool_destroy()`: Destrucción del pool

3. **Estadísticas del Pool**
   - `memory_pool_get_total_allocs()`: Total de asignaciones
   - `memory_pool_get_total_frees()`: Total de liberaciones
   - `memory_pool_dumpStats()`: Visualización de estadísticas

### Tracking Global

El sistema mantiene estadísticas globales de memoria:

1. **Contadores**
   - `memory_get_global_alloc_count()`: Total de asignaciones globales
   - `memory_get_global_free_count()`: Total de liberaciones globales

### Garbage Collection Opcional

El sistema incluye un garbage collector opcional (activado con USE_GC):

1. **Estructura de Control**

   ```c
   typedef struct GCHeader {
       atomic_size_t refCount;
   } GCHeader;
   ```

2. **Funciones del GC**
   - `memory_alloc_gc()`: Asignación gestionada por GC
   - `memory_inc_ref()`: Incremento de contador de referencias
   - `memory_dec_ref()`: Decremento y liberación automática

### Características

1. **Seguridad**

   - Verificación de límites
   - Manejo de errores
   - Limpieza automática

2. **Eficiencia**

   - Pooling para objetos frecuentes
   - Reutilización de memoria
   - Alineación optimizada

3. **Monitoreo**
   - Estadísticas detalladas
   - Niveles de depuración
   - Tracking de uso

### Integración

El sistema se integra con:

- Sistema de errores para reportes
- Sistema de logging para monitoreo
- Sistema de tipos para gestión de objetos
- Sistema de AST para estructuras de datos

## Sistema de Errores

### Descripción General

El sistema de manejo de errores implementa un mecanismo robusto para la detección, reporte y seguimiento de errores en el compilador. El sistema está diseñado para proporcionar información detallada sobre los errores, incluyendo su ubicación, contexto y tipo.

### Tipos de Errores

El sistema clasifica los errores en las siguientes categorías:

```c
typedef enum {
    ERROR_NONE = 0,      // Sin error
    ERROR_SYNTAX,        // Errores de sintaxis
    ERROR_SEMANTIC,      // Errores semánticos
    ERROR_TYPE,          // Errores de tipo
    ERROR_NAME,          // Errores de nombres (identificadores)
    ERROR_MEMORY,        // Errores de memoria
    ERROR_IO,            // Errores de entrada/salida
    ERROR_LIMIT,         // Errores de límites
    ERROR_UNDEFINED,     // Errores indefinidos
    ERROR_RUNTIME,       // Errores en tiempo de ejecución
    ERROR_MAX            // Límite superior
} ErrorType;
```

### Información de Error

Cada error incluye información detallada sobre su ocurrencia:

```c
typedef struct {
    int line;            // Línea donde ocurrió el error
    int column;          // Columna donde ocurrió el error
    const char* file;    // Archivo donde ocurrió el error
    char* message;       // Mensaje descriptivo del error
    char* context;       // Línea de código con el error
    int contextLength;   // Longitud del contexto
    int errorPosition;   // Posición del error en el contexto
    ErrorType type;      // Tipo de error
} ErrorInfo;
```

### Funcionalidades

1. **Reporte de Errores**

   - `error_report()`: Reporta un error con detalles de ubicación
   - `error_set_source()`: Establece el código fuente para contexto
   - `error_print_current()`: Imprime el error actual con contexto

2. **Información de Errores**

   - `error_get_count()`: Obtiene el número total de errores
   - `error_get_last()`: Obtiene el último error reportado
   - `get_error_message()`: Obtiene mensaje descriptivo por tipo

3. **Depuración**
   - `error_push_debug()`: Agrega información de depuración al stack trace

### Características

1. **Contexto**

   - Línea y columna exacta del error
   - Archivo fuente donde ocurrió
   - Contexto del código alrededor del error
   - Posición específica del error

2. **Trazabilidad**

   - Stack trace para seguimiento
   - Información de depuración
   - Historial de errores

3. **Mensajes**
   - Mensajes descriptivos por tipo
   - Contexto del código
   - Ubicación precisa

### Integración

El sistema se integra con:

- Sistema de logging para registro de errores
- Sistema de parser para errores sintácticos
- Sistema de tipos para errores de tipo
- Sistema de memoria para errores de memoria
- Sistema de AST para errores semánticos

## Sistema de Logging

### Descripción General

El sistema de logging implementa un mecanismo flexible para el registro de eventos y mensajes durante la compilación y ejecución del programa. El sistema está diseñado para proporcionar diferentes niveles de detalle en los mensajes y permitir la configuración del destino de los logs.

### Niveles de Log

El sistema implementa cuatro niveles de logging:

```c
typedef enum {
    LOG_DEBUG,   // Mensajes de depuración detallados
    LOG_INFO,    // Información general
    LOG_WARNING, // Advertencias
    LOG_ERROR    // Errores
} LogLevel;
```

### Funcionalidades

1. **Inicialización y Configuración**

   - `logger_init()`: Inicialización con archivo de salida
   - `logger_set_level()`: Configuración del nivel de log
   - `logger_get_level()`: Obtención del nivel actual
   - `logger_close()`: Cierre del sistema de logging

2. **Registro de Mensajes**
   - `logger_log()`: Registro de mensajes con formato variable
   - Soporte para diferentes niveles de log
   - Formato flexible con printf-style

### Características

1. **Flexibilidad**

   - Niveles configurables
   - Formato de mensajes personalizable
   - Destino de salida configurable

2. **Usabilidad**

   - API simple y directa
   - Formato printf familiar
   - Niveles de log intuitivos

3. **Integración**
   - Inicialización automática
   - Cierre limpio
   - Estado persistente

### Integración

El sistema se integra con:

- Sistema de errores para registro de errores
- Sistema de memoria para tracking de memoria
- Sistema de tipos para información de tipos
- Sistema de parser para información de compilación
- Sistema de AST para información de análisis

## Sistema de Módulos

### Descripción General

El sistema de módulos implementa un mecanismo para la organización y gestión de código en unidades modulares. El sistema permite la importación y exportación de símbolos, manejo de dependencias y resolución de nombres, proporcionando una estructura clara para la organización del código.

### Estructuras Principales

1. **Símbolo Exportado**

   ```c
   typedef struct ExportedSymbol {
       char name[256];         // Nombre del símbolo
       AstNode* node;          // Nodo AST correspondiente
       Type* type;             // Tipo del símbolo
       bool isPublic;          // Visibilidad pública
   } ExportedSymbol;
   ```

2. **Módulo Importado**

   ```c
   typedef struct ImportedModule {
       char name[256];         // Nombre del módulo
       char alias[256];        // Alias opcional
       bool isQualified;       // Importación calificada
       struct Module* module;  // Referencia al módulo
   } ImportedModule;
   ```

3. **Módulo**
   ```c
   typedef struct Module {
       char name[256];         // Nombre del módulo
       char path[1024];        // Ruta del archivo
       ExportedSymbol* exports;// Símbolos exportados
       int exportCount;
       ImportedModule* imports;// Módulos importados
       int importCount;
       char** dependencies;    // Dependencias
       int dependencyCount;
       bool isLoaded;          // Estado de carga
       bool isLoading;         // Detección de ciclos
       AstNode* ast;           // AST del módulo
   } Module;
   ```

### Funcionalidades

1. **Gestión del Sistema**

   - `module_system_init()`: Inicialización del sistema
   - `module_system_cleanup()`: Limpieza de recursos
   - `module_set_debug_level()`: Configuración de depuración
   - `module_get_debug_level()`: Obtención de nivel actual

2. **Carga y Resolución**

   - `module_load()`: Carga de módulos
   - `module_import()`: Importación simple
   - `module_import_with_alias()`: Importación con alias
   - `module_resolve_symbol()`: Resolución de símbolos
   - `module_resolve_qualified_symbol()`: Resolución calificada

3. **Gestión de Símbolos**

   - `module_add_export()`: Exportación de símbolos
   - `module_find_export()`: Búsqueda de exportados
   - `module_add_dependency()`: Gestión de dependencias
   - `module_detect_circular_dependency()`: Detección de ciclos

4. **Diagnóstico**
   - `module_count_loaded()`: Conteo de módulos cargados
   - `module_get_name()`: Obtención de nombre
   - `module_get_by_name()`: Búsqueda por nombre
   - `module_print_info()`: Información del módulo
   - `module_set_search_paths()`: Configuración de rutas

### Características

1. **Organización**

   - Sistema de namespaces
   - Control de visibilidad
   - Gestión de dependencias
   - Detección de ciclos

2. **Flexibilidad**

   - Importaciones calificadas
   - Alias para módulos
   - Rutas de búsqueda configurables
   - Exportaciones públicas/privadas

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de errores
   - Sistema de logging

### Integración

El sistema se integra con:

- Sistema de AST para representación del código
- Sistema de tipos para verificación de tipos
- Sistema de errores para reportes
- Sistema de logging para diagnóstico
- Sistema de memoria para gestión de recursos

## Sistema de Aspectos

### Aspectos

(Se completará con los aspectos)

### Pointcuts

(Se completará con los pointcuts)

### Advice

(Se completará con el advice)

## Sistema de Macros

### Descripción General

El sistema de macros implementa un mecanismo de expansión de código en tiempo de compilación. Permite la definición de macros, operadores de stringify (#) y concatenación (##), y la expansión de llamadas a macro.

### Funcionalidades

1. **Gestión del Sistema**

   - `macro_init()`: Inicialización del sistema
   - `macro_set_debug_level()`: Configuración de depuración
   - `macro_cleanup()`: Limpieza de recursos

2. **Gestión de Macros**

   - `register_macro()`: Registro de nuevas macros
   - `expand_macro()`: Expansión de llamadas a macro
   - `evaluate_macros()`: Evaluación de todas las macros

3. **Operadores de Macro**
   - `macro_stringify()`: Operador # (stringify)
   - `macro_concat()`: Operador ## (concatenación)

### Características

1. **Expansión de Código**

   - Registro de macros
   - Expansión de llamadas
   - Evaluación de expresiones
   - Manipulación de strings

2. **Operadores**

   - Stringify (#)
   - Concatenación (##)
   - Expansión de argumentos
   - Manipulación de tokens

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de logging
   - Sistema de errores

## Sistema de Reflexión

### Descripción General

El sistema de reflexión proporciona capacidades de introspección y manipulación de tipos en tiempo de ejecución. Permite examinar y manipular la estructura de tipos, objetos y sus miembros de manera dinámica.

### Estructura de Tipos en Tiempo de Ejecución

```c
typedef struct RuntimeType {
    TypeInfo* typeInfo;    // Información del tipo
    void* vtable;         // Tabla de métodos virtuales
    void* metadata;       // Metadatos adicionales del tipo
} RuntimeType;
```

### Funcionalidades

1. **Introspección de Tipos**

   - `get_runtime_type()`: Obtención del tipo en tiempo de ejecución
   - `is_instance_of()`: Verificación de instancia de tipo
   - `create_instance()`: Creación de instancias de tipo
   - `type_to_string()`: Conversión de tipo a string
   - `print_type_info()`: Impresión de información de tipo

2. **Manipulación de Objetos**

   - `set_field()`: Establecimiento de valores de campos
   - `get_field()`: Obtención de valores de campos
   - `invoke_method()`: Invocación de métodos
   - `has_method()`: Verificación de existencia de métodos
   - `has_field()`: Verificación de existencia de campos

3. **Verificación de Tipos**
   - `is_subtype()`: Verificación de subtipos
   - `is_interface()`: Verificación de interfaces
   - `implements_interface()`: Verificación de implementación de interfaces
   - `get_type_info_by_name()`: Obtención de información por nombre

### Características

1. **Capacidades de Reflexión**

   - Introspección de tipos
   - Manipulación de objetos
   - Invocación dinámica de métodos
   - Acceso a metadatos

2. **Verificación de Tipos**

   - Sistema de tipos en tiempo de ejecución
   - Verificación de subtipos
   - Soporte para interfaces
   - Información de tipos detallada

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de memoria
   - Sistema de logging

## Sistema de Tabla de Símbolos

### Descripción General

El sistema de tabla de símbolos implementa un mecanismo para el seguimiento y gestión de símbolos durante la compilación. Proporciona un sistema de ámbitos anidados y resolución de nombres.

### Estructuras Principales

1. **Símbolo**

   ```c
   typedef struct Symbol {
       char name[256];     // Nombre del símbolo
       Type* type;        // Tipo del símbolo
       int scope;         // Ámbito del símbolo
       struct Symbol* next; // Siguiente símbolo en la lista
   } Symbol;
   ```

2. **Tabla de Símbolos**
   ```c
   typedef struct SymbolTable {
       Symbol* head;      // Lista de símbolos
       int currentScope;  // Ámbito actual
   } SymbolTable;
   ```

### Funcionalidades

1. **Gestión de Tabla**

   - `symbolTable_create()`: Creación de tabla
   - `symbolTable_free()`: Liberación de recursos
   - `symbolTable_set_debug_level()`: Configuración de depuración
   - `symbolTable_get_count()`: Conteo de símbolos
   - `symbolTable_dump()`: Impresión de tabla

2. **Gestión de Ámbitos**

   - `symbolTable_enterScope()`: Entrada a nuevo ámbito
   - `symbolTable_exitScope()`: Salida del ámbito actual

3. **Gestión de Símbolos**
   - `symbolTable_add()`: Agregado de símbolos
   - `symbolTable_lookup()`: Búsqueda en todos los ámbitos
   - `symbolTable_lookupCurrentScope()`: Búsqueda en ámbito actual

### Características

1. **Gestión de Ámbitos**

   - Ámbitos anidados
   - Resolución de nombres
   - Visibilidad de símbolos
   - Control de acceso

2. **Resolución de Nombres**

   - Búsqueda en múltiples ámbitos
   - Precedencia de declaraciones
   - Manejo de sombreado
   - Verificación de tipos

3. **Integración**
   - Sistema de tipos
   - Sistema de errores
   - Sistema de logging
   - Sistema de AST

## Sistema de Templates

### Descripción General

El sistema de templates implementa un mecanismo de programación genérica que permite la creación de código reutilizable con tipos parametrizados. El sistema soporta restricciones de tipo y especialización de código genérico.

### Estructuras Principales

1. **Parámetro de Template**

   ```c
   typedef struct {
       char name[256];         // Nombre del parámetro
       Type* constraint;       // Restricción de tipo (opcional)
   } TemplateParam;
   ```

2. **Definición de Template**

   ```c
   typedef struct {
       char name[256];         // Nombre del template
       TemplateParam** params; // Lista de parámetros
       int paramCount;        // Número de parámetros
       AstNode* body;         // Cuerpo del template
   } TemplateDefinition;
   ```

3. **Instancia de Template**
   ```c
   typedef struct {
       const char* templateName; // Nombre del template
       Type** typeArgs;         // Argumentos de tipo
       int typeArgCount;        // Número de argumentos
   } TemplateInstance;
   ```

### Funcionalidades

1. **Gestión de Templates**

   - `register_template()`: Registro de nuevos templates
   - `instantiate_template()`: Instanciación de templates
   - `optimize_template()`: Optimización de código genérico
   - `validate_template_constraints()`: Validación de restricciones

2. **Manipulación de AST**
   - `clone_ast_node()`: Clonación de nodos AST
   - `substitute_type_params()`: Sustitución de parámetros
   - `inline_template_calls()`: Inline de llamadas a template
   - `specialize_generic_code()`: Especialización de código

### Características

1. **Programación Genérica**

   - Parámetros de tipo
   - Restricciones de tipo
   - Instanciación de templates
   - Especialización de código

2. **Optimización**

   - Inline de llamadas
   - Especialización de código
   - Validación de restricciones
   - Optimización de código genérico

3. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de optimización
   - Sistema de logging

## Sistema de Árbol de Sintaxis Abstracta (AST)

### Descripción General

El sistema de Árbol de Sintaxis Abstracta (AST) implementa una estructura de datos jerárquica que representa el código fuente de manera abstracta. El sistema está diseñado para ser flexible y extensible, permitiendo la representación de diversas características del lenguaje.

### Tipos de Nodos

El sistema implementa una amplia variedad de nodos AST:

1. **Declaraciones de Nivel Superior**

   - `AST_PROGRAM`: Nodo raíz del programa
   - `AST_FUNC_DEF`: Definición de funciones
   - `AST_CLASS_DEF`: Definición de clases
   - `AST_VAR_DECL`: Declaración de variables
   - `AST_IMPORT`: Importación de módulos
   - `AST_MODULE_DECL`: Declaración de módulos
   - `AST_ASPECT_DEF`: Definición de aspectos

2. **Sentencias**

   - `AST_BLOCK`: Bloques de código
   - `AST_IF_STMT`: Estructuras condicionales
   - `AST_FOR_STMT`: Bucles for (tres tipos: range, collection, traditional)
   - `AST_WHILE_STMT`: Bucles while
   - `AST_DO_WHILE_STMT`: Bucles do-while
   - `AST_SWITCH_STMT`: Estructuras switch
   - `AST_CASE_STMT`: Casos de switch
   - `AST_RETURN_STMT`: Sentencias return
   - `AST_VAR_ASSIGN`: Asignaciones de variables
   - `AST_PRINT_STMT`: Sentencias print
   - `AST_BREAK_STMT`: Sentencias break
   - `AST_CONTINUE_STMT`: Sentencias continue
   - `AST_TRY_CATCH_STMT`: Manejo de excepciones
   - `AST_THROW_STMT`: Lanzamiento de excepciones

3. **Expresiones**

   - `AST_BINARY_OP`: Operaciones binarias
   - `AST_UNARY_OP`: Operaciones unarias
   - `AST_NUMBER_LITERAL`: Literales numéricos
   - `AST_STRING_LITERAL`: Literales de cadena
   - `AST_BOOLEAN_LITERAL`: Literales booleanos
   - `AST_NULL_LITERAL`: Literal null
   - `AST_IDENTIFIER`: Identificadores
   - `AST_MEMBER_ACCESS`: Acceso a miembros
   - `AST_ARRAY_ACCESS`: Acceso a arrays
   - `AST_ARRAY_LITERAL`: Literales de array
   - `AST_FUNC_CALL`: Llamadas a funciones
   - `AST_LAMBDA`: Funciones lambda
   - `AST_FUNC_COMPOSE`: Composición de funciones
   - `AST_CURRY_EXPR`: Currying de funciones
   - `AST_NEW_EXPR`: Instanciación de objetos
   - `AST_THIS_EXPR`: Referencia this

4. **Características Avanzadas**
   - `AST_POINTCUT`: Pointcuts para aspectos
   - `AST_ADVICE`: Advice para aspectos
   - `AST_PATTERN_MATCH`: Pattern matching
   - `AST_PATTERN_CASE`: Casos de pattern matching

### Estructura de Nodos

Cada nodo AST tiene una estructura base común:

```c
typedef struct AstNode {
    AstNodeType type;          // Tipo del nodo
    int line;                  // Línea en el código fuente
    int col;                   // Columna en el código fuente
    struct Type* inferredType; // Tipo inferido
    union {
        // Estructuras específicas para cada tipo de nodo
        // ...
    } data;
} AstNode;
```

### Características Especiales

1. **Programación Orientada a Aspectos**

   - Soporte para pointcuts y advice
   - Tipos de advice: before, after, around

2. **Pattern Matching**

   - Soporte para match expressions
   - Casos de pattern matching

3. **Bucles For**

   - Tres tipos de bucles for:
     - Range-based: `for i in range(start, end)`
     - Collection-based: `for elem in collection`
     - Traditional: `for (init; condition; update)`

4. **Inferencia de Tipos**
   - Cada nodo puede almacenar su tipo inferido
   - Integración con el sistema de tipos

### Integración

El sistema se integra con:

- Sistema de tipos para inferencia y verificación
- Sistema de parser para construcción
- Sistema de optimización para transformaciones
- Sistema de generación de código

## Sistema de Tipos AST

### Descripción General

El sistema de tipos AST implementa las funciones de inferencia y verificación de tipos para el Árbol de Sintaxis Abstracta. Este sistema es responsable de asignar tipos a los nodos AST y validar la corrección de tipos en las operaciones.

### Funcionalidades

1. **Gestión de Tipos**
   - `ast_set_type()`: Asignación de tipo a un nodo AST
   - `ast_infer_type()`: Inferencia de tipo para un nodo AST
   - `check_binary_op_types()`: Verificación de tipos en operaciones binarias
   - `validate_ast_types()`: Validación de tipos en todo el AST

### Características

1. **Inferencia de Tipos**

   - Asignación automática de tipos
   - Inferencia basada en contexto
   - Verificación de operaciones
   - Validación de tipos

2. **Integración**
   - Sistema de AST
   - Sistema de tipos
   - Sistema de errores
   - Sistema de logging

## Punto de Entrada

### Descripción General

El punto de entrada del compilador implementa la lógica principal de compilación, incluyendo la gestión de argumentos de línea de comandos, la lectura de archivos fuente, la compilación a C y la generación de ejecutables.

### Funcionalidades

1. **Gestión de Configuración**

   - `set_global_debug_level()`: Configuración del nivel de depuración global
   - Niveles de depuración (0-3)
   - Niveles de optimización (0-2)

2. **Gestión de Archivos**

   - `readFile()`: Lectura de archivos fuente
   - `compileOutputC()`: Compilación a ejecutable
   - Manejo de errores de I/O
   - Gestión de memoria

3. **Interfaz de Usuario**
   - `print_usage()`: Muestra información de uso
   - `print_version()`: Muestra información de versión
   - Opciones de línea de comandos:
     - `-d <level>`: Nivel de depuración
     - `-o <level>`: Nivel de optimización
     - `-h`: Ayuda
     - `-v`: Versión

### Características

1. **Compilación**

   - Lectura de archivos fuente
   - Compilación a C
   - Generación de ejecutables
   - Manejo de errores

2. **Configuración**

   - Niveles de depuración
   - Niveles de optimización
   - Opciones de compilación
   - Gestión de rutas

3. **Integración**
   - Sistema de logging
   - Sistema de errores
   - Sistema de memoria
   - Sistema de tipos
   - Sistema de optimización

### Flujo de Compilación

1. **Inicialización**

   - Configuración del logger
   - Procesamiento de argumentos
   - Configuración de niveles

2. **Lectura y Análisis**

   - Lectura del archivo fuente
   - Análisis léxico
   - Análisis sintáctico
   - Construcción del AST

3. **Procesamiento**

   - Verificación de tipos
   - Optimización
   - Generación de código C

4. **Generación**
   - Compilación a C
   - Generación de ejecutable
   - Limpieza de recursos
