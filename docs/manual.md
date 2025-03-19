# Manual del Lenguaje Lyn

## Índice

1. [Introducción](#introducción)
2. [Sintaxis Básica](#sintaxis-básica)
3. [Tipos de Datos](#tipos-de-datos)
4. [Variables y Asignaciones](#variables-y-asignaciones)
5. [Operadores](#operadores)
6. [Estructuras de Control](#estructuras-de-control)
7. [Funciones](#funciones)
8. [Programación Orientada a Objetos](#programación-orientada-a-objetos)
9. [Manejo de Errores](#manejo-de-errores)
10. [Módulos e Importaciones](#módulos-e-importaciones)
11. [Características Avanzadas](#características-avanzadas)
12. [Buenas Prácticas](#buenas-prácticas)
13. [Ejemplos Completos](#ejemplos-completos)

## Introducción

Lyn es un lenguaje de programación moderno que combina la legibilidad de Python con un sistema de tipos estático y características avanzadas de lenguajes como C++ y JavaScript. Su compilación a código C permite un alto rendimiento y excelente portabilidad.

### Características principales

- **Sintaxis limpia y expresiva**: Diseñada para ser fácil de leer y escribir
- **Tipado estático con inferencia**: Seguridad de tipos sin sintaxis excesiva
- **Orientación a objetos**: Soporte completo para clases y herencia
- **Funciones como ciudadanos de primera clase**: Lambdas y funciones de orden superior
- **Gestión de errores robusta**: Sistema de excepciones try-catch-finally
- **Compilación a C**: Rendimiento óptimo y compatibilidad con código existente

## Sintaxis Básica

### Estructura de un Programa

Todo programa Lyn comienza con la palabra clave `main` y termina con `end`:

```
main
    // Código aquí
end
```

### Comentarios

Lyn soporta comentarios de línea y bloque:

```
// Comentario de una línea

/*
   Comentario
   de múltiples
   líneas
*/
```

### Punto y coma (;)

Los punto y coma son opcionales al final de cada sentencia:

```
// Estas dos líneas son equivalentes
x = 10
x = 10;
```

### Estructura de Bloques

Los bloques de código se delimitan con palabras clave específicas y finalizan con `end`:

```
if x > 10
    // Bloque if
end

for i in range(1, 10)
    // Bloque for
end

func miFuncion()
    // Bloque función
end

class MiClase
    // Bloque clase
end
```

## Tipos de Datos

### Tipos Primitivos

Lyn ofrece los siguientes tipos primitivos:

```
// Enteros
edad = 25

// Números de punto flotante
precio = 29.99

// Booleanos
estaActivo = true
estaInactivo = false

// Cadenas de texto
nombre = "Ana"
mensaje = "Hola, mundo!"
```

### Tipos Compuestos

#### Arrays

```
// Array de enteros
numeros = [1, 2, 3, 4, 5]

// Array bidimensional
matriz = [[1, 2], [3, 4], [5, 6]]

// Acceso a elementos
primerNumero = numeros[0]
elemento = matriz[1][0]  // Accede al valor 3
```

#### Clases y Objetos

```
class Punto
    x float
    y float

    func init(self: Punto, x: float, y: float) -> void
        self.x = x
        self.y = y
    end
end

// Crear instancia
p = Punto()
Punto_init(p, 10.0, 20.0)
```

## Variables y Asignaciones

### Declaración e Inicialización

En Lyn, puedes declarar variables con o sin tipo explícito:

```
// Declaración con tipo explícito
x: int = 10

// Declaración con inferencia de tipo
y = 20       // Se infiere como int
z = 3.14     // Se infiere como float
nombre = "Lyn"   // Se infiere como string
```

### Constantes

Por convención, las constantes se nombran en mayúsculas:

```
PI = 3.14159
GRAVEDAD = 9.81
MAX_USUARIOS = 100
```

### Ámbito de Variables

Las variables declaradas dentro de bloques tienen ámbito local:

```
main
    x = 10  // Variable global

    if true
        y = 20  // Variable local al bloque if
        x = 30  // Modifica la variable global
    end

    // y no está disponible aquí
    // x vale 30
end
```

## Operadores

### Operadores Aritméticos

```
suma = 5 + 3        // 8
resta = 10 - 4      // 6
producto = 3 * 7    // 21
division = 20 / 4   // 5.0
```

### Operadores de Comparación

```
a = 5
b = 10

igual = a == b      // false
diferente = a != b  // true
mayor = a > b       // false
menor = a < b       // true
mayorIgual = a >= b // false
menorIgual = a <= b // true
```

### Operadores Lógicos

```
x = true
y = false

and_result = x and y  // false
or_result = x or y    // true
not_result = not x    // false
```

## Estructuras de Control

### Condicionales

#### If-Else

```
if edad >= 18
    print("Eres mayor de edad")
else
    print("Eres menor de edad")
end
```

#### If-Elif-Else

```
nota = 85

if nota >= 90
    print("Sobresaliente")
else if nota >= 70
    print("Notable")
else if nota >= 60
    print("Aprobado")
else
    print("Suspenso")
end
```

### Bucles

#### For con range

```
// Imprime números del 1 al 5
for i in range(1, 6)
    print(i)
end

// Range puede tener paso
for i in range(0, 10, 2)  // 0, 2, 4, 6, 8
    print(i)
end
```

#### While

```
contador = 0

while contador < 5
    print(contador)
    contador = contador + 1
end
```

#### Do-While

```
contador = 0

do
    print(contador)
    contador = contador + 1
while contador < 5
end
```

### Switch

```
dia = 3

switch dia
    case 1
        print("Lunes")
    case 2
        print("Martes")
    case 3
        print("Miércoles")
    case 4
        print("Jueves")
    case 5
        print("Viernes")
    default
        print("Fin de semana")
end
```

### Break y Continue

```
for i in range(1, 10)
    if i == 5
        break  // Sale del bucle cuando i es 5
    end

    if i % 2 == 0
        continue  // Salta a la siguiente iteración si i es par
    end

    print(i)  // Solo imprime 1 y 3
end
```

## Funciones

### Declaración de Funciones

```
func suma(a: int, b: int) -> int
    return a + b
end

func saludar(nombre: string) -> void
    print("Hola, " + nombre)
end
```

### Llamadas a Funciones

```
resultado = suma(5, 3)  // 8
saludar("Ana")          // Imprime: Hola, Ana
```

### Funciones con Parámetros por Defecto

```
func saludar(nombre: string, saludo: string = "Hola") -> void
    print(saludo + ", " + nombre)
end

saludar("Ana")          // Imprime: Hola, Ana
saludar("Ana", "Buen día")  // Imprime: Buen día, Ana
```

### Funciones Lambda

```
// Lambda simple
duplicar = (x: int) -> int => x * 2
resultado = duplicar(4)  // 8

// Lambda con múltiples parámetros
sumar = (a: int, b: int) -> int => a + b
resultado = sumar(3, 5)  // 8

// Lambda con bloque
calcular = (x: int) -> int => {
    temp = x * x
    return temp + x
}
resultado = calcular(3)  // 12 (9 + 3)
```

### Funciones como Parámetros

```
func aplicar(valor: int, funcion: (int) -> int) -> int
    return funcion(valor)
end

duplicar = (x: int) -> int => x * 2
triplicar = (x: int) -> int => x * 3

resultado1 = aplicar(5, duplicar)   // 10
resultado2 = aplicar(5, triplicar)  // 15
```

## Programación Orientada a Objetos

### Definición de Clases

```
class Rectangulo
    ancho float
    alto float

    func init(self: Rectangulo, ancho: float, alto: float) -> void
        self.ancho = ancho
        self.alto = alto
    end

    func area(self: Rectangulo) -> float
        return self.ancho * self.alto
    end

    func perimetro(self: Rectangulo) -> float
        return 2 * (self.ancho + self.alto)
    end
end
```

### Instanciación y Uso

```
// Crear instancia
rect = Rectangulo()
Rectangulo_init(rect, 5.0, 3.0)

// Llamar a métodos
area = Rectangulo_area(rect)        // 15.0
perimetro = Rectangulo_perimetro(rect)  // 16.0

// Acceder a atributos
print("Ancho: " + rect.ancho)  // Ancho: 5.0
```

### Herencia

```
class Forma
    nombre string

    func init(self: Forma, nombre: string) -> void
        self.nombre = nombre
    end

    func describir(self: Forma) -> string
        return "Soy una forma llamada " + self.nombre
    end
end

class Circulo: Forma
    radio float

    func init(self: Circulo, nombre: string, radio: float) -> void
        Forma_init(self, nombre)
        self.radio = radio
    end

    func area(self: Circulo) -> float
        return 3.14159 * self.radio * self.radio
    end

    // Sobrescribe el método de la clase base
    func describir(self: Circulo) -> string
        baseDesc = Forma_describir(self)
        return baseDesc + " con radio " + self.radio
    end
end
```

## Manejo de Errores

### Try-Catch-Finally

```
try
    // Código que puede generar excepción
    resultado = 10 / 0  // Error: división por cero
catch error
    // Manejo del error
    print("Error: " + error)
finally
    // Este código siempre se ejecuta
    print("Operación finalizada")
end
```

### Lanzamiento de Excepciones

```
func dividir(a: float, b: float) -> float
    if b == 0
        throw "División por cero"
    end
    return a / b
end

try
    resultado = dividir(10, 0)
catch error
    print("Error capturado: " + error)  // Error capturado: División por cero
end
```

## Módulos e Importaciones

### Definición de Módulos

Un módulo en Lyn es una unidad de organización de código que puede contener funciones, clases, variables y otras definiciones. Los módulos permiten encapsular código y evitar colisiones de nombres.

```
module Matematicas
    PI = 3.14159

    func sumar(a: float, b: float) -> float
        return a + b
    end

    func multiplicar(a: float, b: float) -> float
        return a * b
    end
end
```

### Importación de Módulos

```
// Importar un módulo
import "matematicas"

resultado = Matematicas.sumar(5, 3)
```

### Importación de UI y CSS

```
// Importar interfaz de usuario
ui "interfaz.ui"

// Importar estilos
css "estilos.css"
```

## Características Avanzadas

### Arrays y Manipulación

```
// Crear array
numeros = [1, 2, 3, 4, 5]

// Acceder a elementos
primero = numeros[0]  // 1
ultimo = numeros[4]   // 5

// Modificar elementos
numeros[2] = 30  // [1, 2, 30, 4, 5]

// Tamaño del array
longitud = len(numeros)  // 5

// Arrays bidimensionales
matriz = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

valor = matriz[1][2]  // 6
```

### Manejo de Eventos

```
// Registrar evento de clic en un botón
register_event("click", boton, (evento) -> void => {
    print("Botón pulsado")
})

// Registrar evento con captura de datos
register_event("input", campoTexto, (evento) -> void => {
    print("Texto ingresado: " + evento.value)
})
```

### Interfaces gráficas

```
// Definir interfaz de usuario
ui "formulario.ui"

// Acceder a elementos de la interfaz
botonEnviar = UI.getElementById("btnEnviar")
campoNombre = UI.getElementById("txtNombre")

// Registrar eventos
register_event("click", botonEnviar, (e) -> void => {
    nombre = campoNombre.value
    print("Hola, " + nombre)
})
```

### Sistema de Optimización

El compilador de Lyn incluye un potente sistema de optimización que mejora el rendimiento y la eficiencia del código. Este sistema implementa varias técnicas de optimización modernas:

#### Niveles de Optimización

El compilador ofrece tres niveles de optimización:

### Programación Funcional

#### Currificación

```lyn
// Definición de función currificada
func sumar(a: int) -> (int) -> int
    return (b: int) -> int => a + b
end

// Uso de currificación
suma5 = sumar(5)    // Crea una función que suma 5
resultado = suma5(3) // 8

// Encadenamiento de currying
func multiplicar(a: int)(b: int)(c: int) -> int
    return a * b * c
end

doble = multiplicar(2)      // Currying parcial
porCuatro = doble(2)       // Más currying
resultado = porCuatro(5)    // 20
```

#### Pattern Matching

```lyn
// Pattern matching con tipos de datos
match valor
    when 0 =>
        print("Cero")
    when n if n > 0 =>
        print("Positivo")
    when n if n < 0 =>
        print("Negativo")
    otherwise =>
        print("Otro caso")
end

// Pattern matching con estructuras
match persona
    when {nombre: "Juan", edad: 25} =>
        print("Es Juan de 25")
    when {nombre: n, edad: e} if e > 18 =>
        print(n + " es mayor de edad")
    otherwise =>
        print("No coincide")
end
```

#### Composición de Funciones

```lyn
// Definición de funciones a componer
func duplicar(x: int) -> int
    return x * 2
end

func sumarUno(x: int) -> int
    return x + 1
end

// Composición usando >>
resultado = duplicar >> sumarUno // Crea una nueva función
valor = resultado(5)             // (5 * 2) + 1 = 11

// Encadenamiento de composiciones
procesamiento = filtrar >> mapear >> reducir
```

### Programación Orientada a Aspectos

#### Definición de Aspectos Básicos

```lyn
aspect Logging
    // Define el pointcut
    pointcut metodosBD "*.save*() || *.update*() || *.delete*()"

    // Advice que se ejecuta antes
    advice before metodosBD
        print("Iniciando operación de BD: " + currentMethod())
    end

    // Advice que se ejecuta después
    advice after metodosBD
        print("Operación completada")
    end
end
```

#### Aspectos con Context Binding

```lyn
aspect Security
    pointcut operacionesSensibles "*.delete*() || *.admin*()"

    advice before operacionesSensibles
        if not checkUserPermissions()
            throw "Acceso denegado"
        end
        logAccess(getCurrentUser(), currentMethod())
    end
end
```

#### Aspectos con Around Advice

```lyn
aspect Performance
    pointcut operacionesLentas "*.procesamiento*()"

    advice around operacionesLentas
        startTime = getCurrentTime()
        try
            // Ejecuta el método original
            proceed()
        finally
            endTime = getCurrentTime()
            duration = endTime - startTime
            if duration > 1000
                logSlowOperation(duration)
            end
        end
    end
end
```

#### Ejemplo Completo de AOP

```lyn
// Definición de la clase base
class Usuario
    nombre string
    permisos string[]

    func eliminarCuenta(self: Usuario) -> bool
        // Lógica de eliminación
        return true
    end
end

// Aspecto para auditoría
aspect Auditoria
    pointcut operacionesCriticas "*.eliminar*()"

    advice before operacionesCriticas
        usuario = getCurrentUser()
        operacion = currentMethod()
        logAuditoria("Iniciando " + operacion + " por " + usuario)
    end

    advice after operacionesCriticas
        logAuditoria("Operación completada")
    end

    advice around operacionesCriticas
        if not tienePermisosAdmin()
            throw "Permisos insuficientes"
        end
        proceed()
        notificarAdmins("Operación crítica ejecutada")
    end
end

// Uso del sistema
main
    usuario = Usuario()
    Usuario_init(usuario, "Juan", ["user"])

    // Los aspectos se aplican automáticamente
    try
        Usuario_eliminarCuenta(usuario)
    catch error
        print("Error: " + error)
    end
end
```

## Buenas Prácticas

### Convenciones de Nombrado

- **Variables y funciones**: Utilizar camelCase (miVariable, calcularTotal)
- **Clases**: Utilizar PascalCase (MiClase, Rectangulo)
- **Constantes**: Utilizar SCREAMING_SNAKE_CASE (MAX_VALOR, PI)

### Estructura de Proyectos

```

/proyecto
/src
main.lyn // Punto de entrada
/modulos
matematica.lyn
utilidades.lyn
/ui
principal.ui
formulario.ui
/css
estilos.css

```

### Comentarios y Documentación

```

// Función para calcular el factorial de un número
// Parámetros:
// n: Número del que se calculará el factorial
// Retorna:
// El factorial de n
func factorial(n: int) -> int
if n <= 1
return 1
else
return n \* factorial(n - 1)
end
end

```

## Ejemplos Completos

### Calculadora Básica

```

main
print("Calculadora básica")
print("------------------")

    // Obtener valores
    a = 10
    b = 5

    // Realizar operaciones
    suma = a + b
    resta = a - b
    producto = a * b
    division = a / b

    // Mostrar resultados
    print("a = " + a)
    print("b = " + b)
    print("a + b = " + suma)
    print("a - b = " + resta)
    print("a * b = " + producto)
    print("a / b = " + division)

end

```

### Sistema de Gestión de Biblioteca

```

main
// Definición de clases
class Libro
titulo string
autor string
prestado bool

        func init(self: Libro, titulo: string, autor: string) -> void
            self.titulo = titulo
            self.autor = autor
            self.prestado = false
        end

        func prestar(self: Libro) -> bool
            if self.prestado
                return false
            end
            self.prestado = true
            return true
        end

        func devolver(self: Libro) -> bool
            if not self.prestado
                return false
            end
            self.prestado = false
            return true
        end

        func toString(self: Libro) -> string
            estado = "disponible"
            if self.prestado
                estado = "prestado"
            end
            return self.titulo + " de " + self.autor + " (" + estado + ")"
        end
    end

    // Crear algunos libros
    libro1 = Libro()
    Libro_init(libro1, "Don Quijote", "Miguel de Cervantes")

    libro2 = Libro()
    Libro_init(libro2, "Cien años de soledad", "Gabriel García Márquez")

    // Mostrar información inicial
    print("Biblioteca")
    print("----------")
    print(Libro_toString(libro1))
    print(Libro_toString(libro2))

    // Préstamo de libros
    print("\nPréstamo de libros:")
    if Libro_prestar(libro1)
        print("Se ha prestado: " + libro1.titulo)
    else
        print("No se pudo prestar: " + libro1.titulo)
    end

    // Mostrar estado actualizado
    print("\nEstado actualizado")
    print("----------------")
    print(Libro_toString(libro1))
    print(Libro_toString(libro2))

end

```

### Ejemplo con Manejo de Errores

```

main
func dividir(a: float, b: float) -> float
if b == 0
throw "División por cero no permitida"
end
return a / b
end

    // Ejemplo de uso de try-catch
    print("Calculando divisiones...")

    try
        resultado1 = dividir(10, 2)
        print("10 / 2 = " + resultado1)

        resultado2 = dividir(8, 0)  // Esto generará una excepción
        print("8 / 0 = " + resultado2) // Esta línea no se ejecutará
    catch error
        print("Error: " + error)
    finally
        print("Fin del cálculo")
    end

    print("Programa finalizado")

end

```

### Juego Simple

```

main
// Inicializar juego
intentosMax = 5
intentos = 0
numeroSecreto = 42 // En un juego real sería aleatorio
adivinado = false

    print("¡Adivina el número!")
    print("Tienes " + intentosMax + " intentos para adivinar un número entre 1 y 100.")

    // Bucle principal del juego
    while intentos < intentosMax and not adivinado
        intentos = intentos + 1
        print("\nIntento " + intentos + ":")

        // En un programa real, aquí vendría input del usuario
        // Para este ejemplo, usaremos valores de prueba
        intento = 0

        if intentos == 1
            intento = 50
        else if intentos == 2
            intento = 25
        else if intentos == 3
            intento = 40
        else if intentos == 4
            intento = 45
        else
            intento = 42
        end

        print("Tu intento: " + intento)

        // Comprobar intento
        if intento == numeroSecreto
            print("¡Correcto! ¡Has adivinado el número en " + intentos + " intentos!")
            adivinado = true
        else if intento < numeroSecreto
            print("El número es mayor.")
        else
            print("El número es menor.")
        end
    end

    // Fin del juego
    if not adivinado
        print("\n¡Se acabaron los intentos! El número era " + numeroSecreto)
    end

end

```

Este manual proporciona una referencia completa para el lenguaje Lyn y debería servir como guía para nuevos usuarios y referencia para usuarios experimentados.

## Estado de Desarrollo

### Progreso del Proyecto

El lenguaje de programación Lyn se encuentra actualmente en una fase de desarrollo temprana, con aproximadamente un 35% del diseño total implementado.

#### Características implementadas (100%)

- Sintaxis básica y estructura de programas
- Sistema completo de manejo de errores y depuración
- Tipos primitivos (int, float, string, bool)
- Operaciones aritméticas y lógicas básicas
- Estructuras de control (if-else, for, while)
- Funciones simples y llamadas a funciones

#### Características parcialmente implementadas (50-75%)

- Clases y objetos (sintaxis implementada, herencia básica)
- System de tipos y comprobación de tipos
- Arrays y estructuras de datos compuestas
- Optimizaciones básicas de código

#### Características en desarrollo (25-50%)

- Manejo de excepciones try-catch-finally
- Lambdas y funciones de orden superior
- Módulos e importaciones

#### Características planificadas (0-25%)

- Biblioteca estándar completa
- Integración con interfaces gráficas (UI/CSS)
- Soporte para programación asíncrona
- Generadores e iteradores
- Extensiones de metaprogramación

### Hoja de Ruta

1. **Q2 2023**: Estabilizar el sistema de tipos y las características básicas del lenguaje
2. **Q3 2023**: Completar la implementación de la POO y el manejo de excepciones
3. **Q4 2023**: Desarrollar la biblioteca estándar y mejorar las capacidades de módulos
4. **Q1 2024**: Implementar soporte para GUI y características avanzadas

> **Nota**: Este manual describe tanto las características actualmente implementadas como las que están planificadas para futuras versiones. Algunas funcionalidades descritas pueden no estar disponibles en la versión actual del compilador.
