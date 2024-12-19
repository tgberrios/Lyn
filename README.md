Lyn: El Lenguaje del Futuro
(Opcional: Agrega un logo representativo de Lyn si lo tienes)

Introducción
Lyn es un lenguaje de programación moderno, minimalista y eficiente diseñado para ser sencillo de aprender y potente en su rendimiento. Inspirado en la simplicidad de Python y la eficiencia de lenguajes como C++ y Rust, Lyn busca ofrecer una sintaxis limpia, una ejecución rápida y una versatilidad que lo hace ideal para diversas aplicaciones, desde análisis de datos hasta desarrollo web y aplicaciones de red.

Características Implementadas
Hasta la fecha, Lyn incluye las siguientes funcionalidades básicas:

Declaración y Asignación de Variables
Soporte para variables dinámicas y con tipado opcional.
Condicionales (if-else)
Permite ejecutar bloques de código basados en condiciones.
Bucles (while)
Permite la ejecución repetitiva de bloques de código.
Funciones
Definición de funciones con parámetros y retornos.
Llamadas a funciones definidas por el usuario.
Operadores Aritméticos y de Comparación
Soporte para +, -, \*, /, ==, !=, <, >, <=, >=.
Impresión (print)
Permite la salida de texto y valores al usuario.
Instalación
Requisitos
Compilador de C++: Se requiere un compilador compatible con C++17, como g++ de GCC.
Git: Para clonar el repositorio (opcional pero recomendado).
Pasos de Instalación
Clonar el Repositorio

Si tu proyecto está alojado en GitHub u otra plataforma de control de versiones, clónalo utilizando:

bash
Copiar código
git clone https://github.com/tu-usuario/lyn.git
cd lyn
Compilar el Intérprete

Asegúrate de tener g++ instalado y agregado a tu variable de entorno PATH.

Ejecuta el siguiente comando en el directorio raíz del proyecto:

bash
Copiar código
g++ -std=c++17 -o lyn.exe src/lexer/Lexer.cpp src/parser/Parser.cpp src/ast/Interpreter.cpp src/Lyn.cpp
Notas:

Ruta de Lyn.cpp: Asegúrate de que el archivo principal se llame Lyn.cpp y esté ubicado en el directorio src/.
Dependencias: Si tienes más archivos fuente o dependencias, inclúyelos en el comando de compilación.
Verificar la Instalación

Después de la compilación, verifica que el ejecutable lyn.exe se haya creado correctamente en el directorio raíz.

Uso
Para ejecutar un programa escrito en Lyn, sigue estos pasos:

Crear un Archivo de Código Fuente

Crea un archivo con la extensión .lyn, por ejemplo, program.lyn:

plaintext
Copiar código
x = 42
y = 10
if (x > y) {
print("x is greater than y")
}

func add(a, b) {
return a + b
}

result = add(15, 27)
print("Result of addition: " + result)

counter = 5
while (counter > 0) {
print("Counter: " + counter)
counter = counter - 1
}
Ejecutar el Programa

Usa el intérprete Lyn para ejecutar el archivo:

bash
Copiar código
./lyn.exe program.lyn
Salida Esperada:

vbnet
Copiar código
x set to 42
y set to 10
Print: "x is greater than y"
Function "add" defined.
result set to 42
Print: "Result of addition: 42"
counter set to 5
Print: "Counter: 5"
counter set to 4
Print: "Counter: 4"
counter set to 3
Print: "Counter: 3"
counter set to 2
Print: "Counter: 2"
counter set to 1
Print: "Counter: 1"
counter set to 0
Ejemplos
Ejemplo 1: Condicionales y Bucles
plaintext
Copiar código
x = 10
y = 20
if (x < y) {
print("x is less than y")
} else {
print("x is not less than y")
}

func add(a, b) {
return a + b
}

result = add(10, 20)
print("Result: " + result)

counter = 5
while (counter > 0) {
print("Counter: " + counter)
counter = counter - 1
}
Salida Esperada:

vbnet
Copiar código
x set to 10
y set to 20
Function "add" defined.
x is less than y
result set to 30
Print: "Result: 30"
counter set to 5
Print: "Counter: 5"
counter set to 4
Print: "Counter: 4"
counter set to 3
Print: "Counter: 3"
counter set to 2
Print: "Counter: 2"
counter set to 1
Print: "Counter: 1"
counter set to 0
Ejemplo 2: Funciones y Operadores
plaintext
Copiar código
func multiply(a, b) {
return a \* b
}

z = multiply(6, 7)
print("6 multiplied by 7 is " + z)
Salida Esperada:

vbnet
Copiar código
Function "multiply" defined.
z set to 42
Print: "6 multiplied by 7 is 42"
Roadmap
Lyn está en constante desarrollo, y planeamos implementar las siguientes funcionalidades en el futuro:

Compilación a Código Nativo

Integración con LLVM para generación de código nativo optimizado.
Soporte para WebAssembly para ejecución en navegadores.
Data Analysis Nativo

Implementar tablas de datos similares a DataFrames en Pandas.
Soporte para vectores multidimensionales y matrices.
Integrar funciones para cálculos estadísticos y filtrado de datos.
Machine Learning y AI

Desarrollar interfaces para crear y entrenar modelos de redes neuronales.
Implementar algoritmos clásicos de machine learning.
Integrar soporte para computación paralela usando CUDA/OpenCL.
Networking Nativo

Crear APIs para desarrollar servidores HTTP.
Soporte para sockets y comunicación en red.
Facilitar el desarrollo de aplicaciones distribuidas y microservicios.
Desarrollo Web Integrado

Implementar funciones para manipular el Document Object Model (DOM).
Desarrollar o integrar frameworks que simplifiquen el desarrollo de aplicaciones web.
Asincronía Simplificada

Implementar manejo nativo de tareas asincrónicas usando task y await.
Optimizar la ejecución concurrente y paralela de tareas.
Ecosistema y Herramientas Modernas

Desarrollar una interfaz de línea de comandos (CLI) para compilación, ejecución y gestión de bibliotecas.
Crear un gestor de paquetes para gestionar dependencias y paquetes.
Implementar un editor interactivo en línea similar a Jupyter Notebook.
Extensibilidad

Desarrollar mecanismos para importar y utilizar librerías externas de C++ y Python.
Crear bindings que permitan extender las capacidades de Lyn con código escrito en otros lenguajes.
Optimización y Seguridad

Implementar un sistema de gestión de memoria eficiente y seguro.
Asegurar que el intérprete y el compilador manejen correctamente errores y excepciones.
Documentación y Comunidad

Crear una documentación completa que abarque la sintaxis, las APIs, ejemplos y guías de uso.
Establecer canales de comunicación como foros, chats y repositorios para facilitar la colaboración y el soporte entre usuarios.
Contribuciones
Las contribuciones son bienvenidas. Si deseas contribuir a Lyn, por favor sigue estos pasos:

Fork del Repositorio

Haz un fork del repositorio para tu propio uso.

Crear una Rama Nueva

Crea una rama para tu funcionalidad o corrección de errores:

bash
Copiar código
git checkout -b feature/nueva-funcionalidad
Realizar los Cambios

Implementa tu funcionalidad o corrige los errores.

Commit y Push

Realiza commits claros y push a tu fork:

bash
Copiar código
git commit -m "Descripción de los cambios"
git push origin feature/nueva-funcionalidad
Abrir un Pull Request

Desde tu fork, abre un pull request describiendo tus cambios.

Licencia
Este proyecto está licenciado bajo la Licencia MIT. Consulta el archivo LICENSE para más detalles.

Contacto
Para más información, sugerencias o reportar problemas, por favor contacta a:

Nombre: TomyGustavo
Correo Electrónico: tu-email@example.com
GitHub: tu-usuario
Consideraciones Finales
Imágenes y Recursos: Si tienes un logo o capturas de pantalla de Lyn en acción, puedes agregarlas para mejorar la apariencia del README. Asegúrate de alojarlas en una carpeta dentro del proyecto (por ejemplo, assets/images) y referenciarlas adecuadamente.

Ejemplos Adicionales: A medida que implementes nuevas funcionalidades, actualiza la sección de ejemplos para mostrar cómo usarlas.

Documentación Extendida: Considera crear una sección o un enlace a una documentación más detallada si el proyecto crece en complejidad.

Enlaces a Repositorios: Si tienes repositorios separados para el lexer, parser, intérprete, etc., incluye enlaces o menciona cómo están organizados.

Este README.md proporciona una visión clara y estructurada de lo que es Lyn, qué funcionalidades tiene actualmente, cómo instalarlo y usarlo, y hacia dónde se dirige el proyecto. A medida que avances en el desarrollo de Lyn, puedes continuar actualizando este archivo para reflejar las nuevas características y mejoras.

¡Buena suerte con Lyn: El Lenguaje del Futuro! 🚀
