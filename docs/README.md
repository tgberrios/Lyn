# Lyn – Lenguaje de Alto Rendimiento y Versátil

Lyn es un lenguaje de programación moderno que combina la legibilidad de Python con un sistema de tipos estático y características avanzadas de lenguajes como C++, JavaScript y AspectJ. Su compilación a código C permite un alto rendimiento y excelente portabilidad.

## Características Principales

- **Sintaxis limpia y expresiva**: Diseñada para ser fácil de leer y escribir
- **Tipado estático con inferencia**: Seguridad de tipos sin sintaxis excesiva
- **Orientación a objetos**: Soporte completo para clases y herencia
- **Funciones como ciudadanos de primera clase**: Lambdas y funciones de orden superior
- **Gestión de errores robusta**: Sistema de excepciones try-catch-finally
- **Programación orientada a aspectos**: Soporta aspectos, pointcuts y advice
- **Compilación a C**: Rendimiento óptimo y compatibilidad con código existente

## Estado Actual del Proyecto

### Características Implementadas ✅

1. **Sistema de Tipos**

   - Tipos primitivos (int, float, string, bool)
   - Inferencia de tipos
   - Verificación de tipos estática
   - Compatibilidad de tipos

2. **Estructuras de Control**

   - If-else y switch
   - Bucles while, do-while y for
   - Break y continue
   - Pattern matching básico

3. **Programación Orientada a Objetos**

   - Clases y objetos
   - Métodos y constructores
   - Atributos y encapsulamiento
   - Herencia básica

4. **Programación Orientada a Aspectos**

   - Aspectos y pointcuts
   - Advice (before, after, around)
   - Weaving de aspectos
   - Interceptación de métodos

5. **Sistema de Memoria**
   - Gestión automática de memoria
   - Garbage collection opcional
   - Modo embedded con memory pooling
   - Control manual de memoria

### Características en Desarrollo 🚧

1. **Características Avanzadas**

   - Herencia múltiple
   - Interfaces y traits
   - Generics y templates
   - Macros avanzadas

2. **Interoperabilidad**

   - Integración con JavaScript
   - Bindings para Python
   - Soporte para NPM
   - FFI para C/C++

3. **Sistema de Módulos**
   - Importación de módulos
   - Gestión de dependencias
   - Namespaces
   - Módulos estándar

### Características Planificadas 📅

1. **Paralelización**

   - Hilos y procesos
   - SIMD y vectorización
   - Programación asíncrona
   - Corrutinas

2. **Ecosistema**
   - Gestor de paquetes (lyn_pm)
   - Repositorio central (lyn_hub)
   - Herramientas de desarrollo
   - Debugger integrado

## Ejemplos de Código

### Sintaxis Básica

```lyn
// Variables y tipos
x: int = 10
y = 20  // Inferencia de tipo
texto = "Hola, Lyn!"

// Funciones
func suma(a: int, b: int) -> int
    return a + b;
end

// Clases
class Persona
    nombre: string
    edad: int

    func init(nombre: string, edad: int)
        this.nombre = nombre
        this.edad = edad
    end
end

// Aspectos
aspect LoggingAspect
    pointcut loggedFunctions "test_*"

    advice before loggedFunctions
        print("Antes de ejecutar la función")
    end
end
```

### Características Avanzadas

```lyn
// Pattern Matching
match valor
    when 0:
        print("Cero")
    when n if n > 0:
        print("Positivo")
    otherwise:
        print("Otro caso")
end

// Manejo de Excepciones
try
    resultado = dividir(a, b)
catch (error)
    print("Error: " + error.message)
end

// Composición de Funciones
composed = add_one >> multiply_by_two
result = composed(5)
```

## Documentación

- [Documentación Técnica](docs.md): Detalles técnicos y especificaciones
- [Ejemplos de Código](examples.md): Guía práctica con ejemplos
- [TODO](TODO.md): Estado detallado de características

## Requisitos del Sistema

- Compilador C (gcc/clang)
- Sistema operativo compatible (Linux, macOS, Windows)
- 4GB RAM mínimo
- 1GB espacio en disco

## Instalación

```bash
git clone https://github.com/tu-usuario/lyn.git
cd lyn
make
sudo make install
```

## Uso Básico

```bash
# Compilar un archivo
lync programa.lyn

# Ejecutar el programa
./programa

# Compilar con optimizaciones
lync -O3 programa.lyn

# Modo embedded
lync --embedded programa.lyn
```
