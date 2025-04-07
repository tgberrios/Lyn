# Sistema de Módulos en Lyn

## Estado Actual (Mayo 2024)

El sistema de módulos básico ha sido implementado con las siguientes capacidades:

- Sintaxis básica de importación: `import modulename`
- Acceso a funciones del módulo con notación de punto: `modulename.functionname(args)`
- Generación automática de código para implementaciones básicas de funciones comunes
- Preparación para imports selectivos y aliases (no completamente implementados)

### Implementación Técnica

1. **Análisis Sintáctico**: El parser reconoce la declaración `import` y la procesa correctamente.

2. **Generación de Código**:

   - Implementa directamente las funciones del módulo en el archivo C generado
   - Crea una estructura para permitir la notación de punto
   - Inicializa las referencias a funciones correctamente

3. **Compilación**: El código generado compila sin problemas de referencia o enlace.

### Ejemplo de Funcionamiento

Código Lyn:

```lyn
main
    import math_lib

    a = 5
    b = 3

    // Uso de notación de punto
    result = math_lib.add(a, b)
    print("math_lib.add(5, 3) = " + result)
end
```

Código C generado:

```c
// Implementaciones directas de funciones
double math_lib_add(int contextID, double a, double b) {
    return a + b;
}
// ... más funciones

// Estructura para notación de punto
struct math_lib_Module {
    double (*add)(int, double, double);
    // ... más miembros
};

// Inicialización con punteros constantes
struct math_lib_Module math_lib = {
    /* inicializadores */
};
```

## Próximos Pasos

1. **Importaciones Selectivas**

   - Implementar completamente `import {function1, function2} from module`
   - Soportar aliases para funciones importadas: `import {fn1 as alias1} from module`

2. **Carga de Módulos Reales**

   - Cargar las implementaciones reales desde los archivos de módulo
   - Soportar dependencias anidadas entre módulos

3. **Mejoras en Rendimiento**

   - Optimizar la generación de código para evitar duplicaciones
   - Implementar carga perezosa de módulos

4. **Empaquetado y Distribución**
   - Crear sistema para distribuir módulos como paquetes
   - Implementar resolución de versiones

## Limitaciones Actuales

1. Las implementaciones de funciones son hard-coded para funciones comunes (add, subtract, etc.)
2. No se soporta la importación desde archivos de módulo reales
3. La importación selectiva no está completamente implementada
4. No hay manejo de errores robusto para módulos faltantes o funciones no definidas

## Ruta de Desarrollo

- **Q2 2024**: Mejorar la integración con módulos reales y soportar importación selectiva
- **Q3 2024**: Implementar sistema de carga dinámica de módulos y resolución de dependencias
- **Q4 2024**: Desarrollar sistema de paquetes y distribución
