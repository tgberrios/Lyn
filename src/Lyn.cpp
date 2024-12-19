#include "lexer/Lexer.h"
#include <iostream> // Para usar std::cout y std::endl

int main() {
    // Definimos el código fuente que queremos analizar.
    std::string code = R"(var x = 42; if (x > 10) print("x is big"))";

    
    // Creamos un lexer para el código fuente.
    Lexer lexer(code);

    // Declarar un token 't' que usaremos para ir almacenando el resultado de nextToken().
    Token t;

    // Iniciamos el ciclo do-while para recorrer todos los tokens del código fuente.
    do {
        // Llamamos nextToken() para obtener el siguiente tokSen del lexer.
        t = lexer.nextToken();

        // Imprimimos el token obtenido.
        std::cout << "Token: " << (int)t.type
                  << " Value: " << t.value
                  << " Line: " << t.line
                  << " Col: " << t.column
                  << "\n";

    } while (t.type != TokenType::END_OF_FILE && t.type != TokenType::UNKNOWN);
    return 0;
}
