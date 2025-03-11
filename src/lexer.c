#include "lexer.h"
#include "memory.h"
#include "error.h"  // Añadir este include
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_MEMORY
    #define DBG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
    #define DBG_PRINT(...) /* No hace nada */
#endif

static const char *source;
static int position;
static int line = 1;
static int col = 1;

/**
 * @brief Inicializa el lexer con la fuente de entrada.
 *
 * @param src Puntero a la cadena de entrada.
 */
void lexerInit(const char *src) {
    source = src;
    position = 0;
    line = 1;
    col = 1;
    error_set_source(src);  // Añadido: configura el código fuente para el sistema de errores
}

static void lexerError(const char* message) {
    error_report("lexer", line, col, message);
    error_print_current();
    exit(1);
}

/**
 * @brief Guarda el estado actual del lexer.
 *
 * @return LexerState Estado actual del lexer.
 */
LexerState lexSaveState(void) {
    LexerState state = { source, position, line, col };
    return state;
}

/**
 * @brief Restaura el estado del lexer.
 *
 * @param state Estado a restaurar.
 */
void lexRestoreState(LexerState state) {
    source = state.source;
    position = state.position;
    line = state.line;
    col = state.col;
}

/**
 * @brief Avanza un carácter en la fuente.
 *
 * Incrementa la posición y la columna, y retorna el carácter leído.
 *
 * @return char Carácter avanzado.
 */
static char advance(void) {
    col++;
    return source[position++];
}

/**
 * @brief Retorna el carácter actual sin avanzar la posición.
 *
 * @return char Carácter actual.
 */
static char peek(void) {
    return source[position];
}

/**
 * @brief Omite espacios en blanco, saltos de línea y comentarios.
 *
 * Salta todos los espacios, saltos de línea y comentarios (línea y bloque)
 * para posicionar el lexer en el siguiente token relevante.
 */
static void skipWhitespaceAndComments(void) {
    while (1) {
        /* Saltar espacios y saltos de línea. */
        while (isspace(source[position])) {
            if (source[position] == '\n') {
                line++;
                col = 0;
            }
            advance();
        }
        /* Comentario de línea: // */
        if (source[position] == '/' && source[position + 1] == '/') {
            while (source[position] != '\n' && source[position] != '\0')
                advance();
            continue;
        }
        /* Comentario de bloque: /* ... *\/ */
        if (source[position] == '/' && source[position + 1] == '*') {
            advance(); // Consume '/'
            advance(); // Consume '*'
            while (!(source[position] == '*' && source[position + 1] == '/') && source[position] != '\0') {
                if (source[position] == '\n') {
                    line++;
                    col = 0;
                }
                advance();
            }
            if (source[position] != '\0') {
                advance(); // Consume '*'
                advance(); // Consume '/'
            }
            continue;
        }
        break;
    }
}

/**
 * @brief Obtiene el siguiente token de la fuente.
 *
 * Analiza la entrada y retorna el siguiente token.
 *
 * @return Token Estructura Token con tipo, lexema, línea y columna.
 */
Token getNextToken(void) {
    skipWhitespaceAndComments();

    if (source[position] == '\0') {
        Token token = { TOKEN_EOF, "EOF", line, col };
        return token;
    }

    char c = advance();
    Token token = { 0, "", line, col - 1 };

    /* Manejo de identificadores y palabras clave. */
    if (isalpha(c) || c == '_') {
        int start = position - 1;
        while (isalnum(source[position]) || source[position] == '_')
            advance();
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';

        if (strcmp(token.lexeme, "func") == 0) token.type = TOKEN_FUNC;
        else if (strcmp(token.lexeme, "return") == 0) token.type = TOKEN_RETURN;
        else if (strcmp(token.lexeme, "print") == 0) token.type = TOKEN_PRINT;
        else if (strcmp(token.lexeme, "class") == 0) token.type = TOKEN_CLASS;
        else if (strcmp(token.lexeme, "if") == 0) token.type = TOKEN_IF;
        else if (strcmp(token.lexeme, "else") == 0) token.type = TOKEN_ELSE;
        else if (strcmp(token.lexeme, "for") == 0) token.type = TOKEN_FOR;
        else if (strcmp(token.lexeme, "in") == 0) token.type = TOKEN_IN;
        else if (strcmp(token.lexeme, "end") == 0) token.type = TOKEN_END;
        else if (strcmp(token.lexeme, "import") == 0) token.type = TOKEN_IMPORT;
        else if (strcmp(token.lexeme, "ui") == 0) token.type = TOKEN_UI;
        else if (strcmp(token.lexeme, "css") == 0) token.type = TOKEN_CSS;
        else if (strcmp(token.lexeme, "register_event") == 0) token.type = TOKEN_REGISTER_EVENT;
        else if (strcmp(token.lexeme, "range") == 0) token.type = TOKEN_RANGE;
        else if (strcmp(token.lexeme, "int") == 0) {
            token.type = TOKEN_INT;
            DBG_PRINT("Detected token: int as TOKEN_INT\n");
        }
        else if (strcmp(token.lexeme, "float") == 0) token.type = TOKEN_FLOAT;
        else if (strcmp(token.lexeme, "module") == 0) token.type = TOKEN_MODULE;
        else if (strcmp(token.lexeme, "export") == 0) token.type = TOKEN_EXPORT;
        else token.type = TOKEN_IDENTIFIER;
        return token;
    }

    /* Manejo de números. */
    if (isdigit(c) || (c == '.' && isdigit(peek()))) {
        int start = position - 1;
        while (isdigit(source[position]) || source[position] == '.')
            advance();
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';
        token.type = TOKEN_NUMBER;
        return token;
    }

    /* Manejo de cadenas. */
    if (c == '"') {
        int start = position;
        while (source[position] != '"' && source[position] != '\0') {
            if (source[position] == '\n') {
                lexerError("Unterminated string literal");
            }
            advance();
        }
        if (source[position] == '\0') {
            lexerError("Unterminated string literal");
        }
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';
        advance(); // Consume la comilla de cierre.
        token.type = TOKEN_STRING;
        return token;
    }

    /* Manejo de operadores y símbolos. */
    switch (c) {
        case '=':
            if (peek() == '=') {
                advance();
                token.type = TOKEN_EQ;
                strcpy(token.lexeme, "==");
            } else if (peek() == '>') {
                advance();
                token.type = TOKEN_FAT_ARROW;
                strcpy(token.lexeme, "=>");
            } else {
                token.type = TOKEN_ASSIGN;
                token.lexeme[0] = '=';
                token.lexeme[1] = '\0';
            }
            break;
        case ':':   // <-- NUEVO
            token.type = TOKEN_COLON;
            token.lexeme[0] = ':';
            token.lexeme[1] = '\0';
            break;
        case '+':
            token.type = TOKEN_PLUS;
            token.lexeme[0] = '+';
            token.lexeme[1] = '\0';
            break;
        case '-':
            if (peek() == '>') {
                advance();
                token.type = TOKEN_ARROW;
                strcpy(token.lexeme, "->");
            } else {
                token.type = TOKEN_MINUS;
                token.lexeme[0] = '-';
                token.lexeme[1] = '\0';
            }
            break;
        case '*':
            token.type = TOKEN_ASTERISK;
            token.lexeme[0] = '*';
            token.lexeme[1] = '\0';
            break;
        case '/':
            token.type = TOKEN_SLASH;
            token.lexeme[0] = '/';
            token.lexeme[1] = '\0';
            break;
        case '(':
            token.type = TOKEN_LPAREN;
            token.lexeme[0] = '(';
            token.lexeme[1] = '\0';
            break;
        case ')':
            token.type = TOKEN_RPAREN;
            token.lexeme[0] = ')';
            token.lexeme[1] = '\0';
            break;
        case ',':
            token.type = TOKEN_COMMA;
            token.lexeme[0] = ',';
            token.lexeme[1] = '\0';
            break;
        case '.':
            token.type = TOKEN_DOT;
            token.lexeme[0] = '.';
            token.lexeme[1] = '\0';
            break;
        case ';':
            token.type = TOKEN_SEMICOLON;
            token.lexeme[0] = ';';
            token.lexeme[1] = '\0';
            break;
        case '>':
            if (peek() == '=') {
                advance();
                token.type = TOKEN_GTE;
                strcpy(token.lexeme, ">=");
            } else {
                token.type = TOKEN_GT;
                token.lexeme[0] = '>';
                token.lexeme[1] = '\0';
            }
            break;
        case '<':
            if (peek() == '=') {
                advance();
                token.type = TOKEN_LTE;
                strcpy(token.lexeme, "<=");
            } else {
                token.type = TOKEN_LT;
                token.lexeme[0] = '<';
                token.lexeme[1] = '\0';
            }
            break;
        case '!':
            if (peek() == '=') {
                advance();
                token.type = TOKEN_NEQ;
                strcpy(token.lexeme, "!=");
            } else {
                token.type = TOKEN_UNKNOWN;
                snprintf(token.lexeme, sizeof(token.lexeme), "Unknown character");
            }
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            strcpy(token.lexeme, "[");
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            strcpy(token.lexeme, "]");
            break;
        default:
            token.type = TOKEN_UNKNOWN;
            snprintf(token.lexeme, sizeof(token.lexeme), "Unknown character");
            break;
    }
    return token;
}
