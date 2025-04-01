# Ejemplos de Programación en Lyn

Este documento proporciona ejemplos prácticos de cómo programar en el lenguaje Lyn, cubriendo sus principales características y funcionalidades.

## Índice

1. [Variables y Tipos](#variables-y-tipos)
2. [Operaciones Aritméticas](#operaciones-aritméticas)
3. [Estructuras de Control](#estructuras-de-control)
4. [Funciones](#funciones)
5. [Manejo de Excepciones](#manejo-de-excepciones)
6. [Pattern Matching](#pattern-matching)
7. [Programación Orientada a Aspectos](#programación-orientada-a-aspectos)
8. [Clases y Objetos](#clases-y-objetos)
9. [Tipos Avanzados](#tipos-avanzados)
10. [Macros](#macros)
11. [Composición de Funciones](#composición-de-funciones)

## Variables y Tipos

### Declaración de Variables

```lyn
// Variables básicas
entero = 42               // Integer
decimal = 3.14            // Float
texto = "Hola, Lyn!"      // String

// Declaración explícita de tipos
explicit_int: int = 42
explicit_float: float = 3.14

// Inferencia de tipos
inferred_int = 100        // Se infiere como int
inferred_float = 2.718    // Se infiere como float
inferred_string = "Hello" // Se infiere como string
```

### Operaciones con Strings

```lyn
// Concatenación de strings
mensaje = "Hola " + "Mundo"

// Concatenación con números
numero = 42
texto = "El número es: " + numero
```

## Operaciones Aritméticas

```lyn
// Operaciones básicas
suma = 5 + 3
resta = 10 - 4
producto = 3 * 7
division = 20 / 4

// Operaciones mixtas (int y float)
entero = 42
decimal = 3.14
resultado = entero + decimal  // Se promueve a float
```

## Estructuras de Control

### If-Else

```lyn
// If-else básico
if (valor > 10)
    print("valor es mayor que 10")
else
    print("valor es menor o igual que 10")
end

// If con múltiples condiciones
edad = 25
if (edad >= 18)
    print("Eres adulto")
else
    print("No eres adulto")
end
```

### Bucles While

```lyn
// Bucle while básico
count = 1
while (count <= 5)
    print(count)
    count = count + 1
end

// Bucle do-while
do_while_count = 1
do
    print(do_while_count)
    do_while_count = do_while_count + 1
while (do_while_count <= 5)
end
```

### Bucles For

```lyn
// For con range
for i in range(1, 6)
    print(i)
end

// For con paso
for i in range(0, 10, 2)
    print(i)
end

// For con colección
numeros = [10, 20, 30, 40, 50]
for num in numeros
    print(num)
end

// For tradicional estilo C
for (i = 0; i < 5; i = i + 1)
    print(i)
end
```

### Switch

```lyn
// Switch básico
switch (valor)
    case 1:
        print("uno")
        break
    case 2:
        print("dos")
        break
    default:
        print("otro")
end

// Switch con fall-through
switch (valor)
    case 1:
        print("uno, ")
        // Sin break, continúa al siguiente caso
    case 2:
        print("dos, ")
    case 3:
        print("tres")
        break
end
```

## Funciones

```lyn
// Función con tipo de retorno explícito
func add(a: int, b: int) -> int
    return a + b;
end

// Función con strings
func greet(name: string) -> string
    return "Hola, " + name;
end

// Función con arrow syntax
func multiply(a: int, b: int) => a * b;

// Llamada a funciones
suma = add(5, 3)
saludo = greet("Mundo")
```

## Manejo de Excepciones

```lyn
// Bloque try-catch básico
try
    resultado = dividir(a, b)
catch (error)
    print("Error: " + error.message)
end

// Bloque try-catch-finally
try
    archivo = abrir_archivo("datos.txt")
    procesar_datos(archivo)
catch (error)
    print("Error al procesar archivo: " + error.message)
finally
    cerrar_archivo(archivo)
end

// Lanzamiento de excepciones
func dividir(a: int, b: int) -> float
    if (b == 0)
        throw "División por cero no permitida"
    end
    return a / b;
end
```

## Pattern Matching

```lyn
// Pattern matching básico
match (valor)
    when 1:
        print("uno")
    when 2:
        print("dos")
    when n if n > 10:
        print("mayor que 10")
    otherwise:
        print("otro valor")
end

// Pattern matching con tipos
match (objeto)
    when (x: int):
        print("es un entero: " + x)
    when (s: string):
        print("es un string: " + s)
    otherwise:
        print("tipo desconocido")
end
```

## Programación Orientada a Aspectos

```lyn
// Definición de un aspecto
aspect LoggingAspect
    // Definición de pointcut
    pointcut loggedFunctions "test_*"

    // Advice antes de la ejecución
    advice before loggedFunctions
        print("Antes de ejecutar la función")
    end

    // Advice después de la ejecución
    advice after loggedFunctions
        print("Después de ejecutar la función")
    end

    // Advice alrededor de la ejecución
    advice around loggedFunctions
        print("Antes del around")
        proceed()  // Continúa con la ejecución original
        print("Después del around")
    end
end

// Función que será asesorada por el aspecto
func test_function()
    print("Dentro de test_function")
end
```

## Clases y Objetos

```lyn
// Definición de clase
class Car
    brand = "Toyota"
    print("Creando un Car con marca = " + brand)
end

// Creación de instancia
var myCar = new Car()
print("Mi carro es de marca: " + myCar.brand)

// Clase con métodos
class Person
    name = ""
    age = 0

    func init(name: string, age: int)
        this.name = name
        this.age = age
    end

    func greet() -> string
        return "Hola, soy " + this.name
    end
end
```

## Tipos Avanzados

### Operaciones Booleanas

```lyn
// Variables booleanas
bool_val1 = true
bool_val2 = false

// Operaciones lógicas
bool_and = bool_val1 and bool_val2
bool_or = bool_val1 or bool_val2

// Comparaciones
is_greater = 10 > 5
is_equal = 7 == 7
```

### Conversiones de Tipo

```lyn
// Conversión automática en expresiones mixtas
int_val = 42
float_val = 3.14
mixed_expr = int_val + float_val  // Se convierte automáticamente a float

// Concatenación con conversión numérica
str_numeric = "La respuesta es: " + int_val
```

## Macros

```lyn
// Definición de macro
macro MAX(a, b)
    if (a > b)
        a
    else
        b
    end
end

// Uso de macro
max_val = MAX(10, 20)

// Macro con stringification
macro DEBUG(msg)
    print("DEBUG: " + #msg)
end

// Macro con concatenación
macro MAKE_ID(prefix, num)
    prefix##num
end

// Expansión de macro
var MAKE_ID(user_, 1) = "John"
```

## Composición de Funciones

```lyn
// Funciones básicas
func add_one(x: int) -> int
    return x + 1;
end

func multiply_by_two(x: int) -> int
    return x * 2;
end

// Composición de funciones usando >>
composed = add_one >> multiply_by_two
result = composed(5)  // Resultado: 12 (5 + 1 = 6, 6 * 2 = 12)
```

## Consejos y Buenas Prácticas

1. **Inferencia de Tipos**

   - Lyn puede inferir tipos automáticamente
   - Usa declaraciones explícitas cuando sea necesario para claridad
   - Los tipos se verifican en tiempo de compilación

2. **Estructuras de Control**

   - Usa `end` para cerrar bloques
   - Los paréntesis son opcionales en condiciones simples
   - `break` es necesario en `switch` para evitar fall-through
   - Usa `;` al final de las sentencias dentro de funciones

3. **Aspectos**

   - Define pointcuts claros y específicos
   - Usa advice para funcionalidad transversal
   - Mantén los aspectos simples y enfocados
   - `proceed()` es necesario en advice `around`

4. **Clases**

   - Usa `new` para crear instancias
   - Accede a miembros con notación de punto
   - Los constructores se ejecutan automáticamente
   - Usa `this` para referenciar la instancia actual

5. **Manejo de Errores**

   - Usa el sistema de tipos para prevenir errores
   - Valida entradas en funciones
   - Maneja casos especiales apropiadamente
   - Usa try-catch para manejar excepciones

6. **Macros**

   - Usa `#` para stringification
   - Usa `##` para concatenación
   - Las macros se expanden en tiempo de compilación
   - Evita macros complejas que dificulten el debugging

7. **Pattern Matching**
   - Usa `when` para patrones específicos
   - Usa `otherwise` como caso por defecto
   - Puedes usar guardas con `if` en los patrones
   - Soporta matching de tipos y valores
