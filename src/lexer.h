#ifndef LYN_LEXER_H
#define LYN_LEXER_H

// Añadir la inclusión del sistema de errores
#include "error.h"
#include "logger.h"  // Añadimos el logger

/**
 * Enumeración de los tipos de tokens.
 */
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,      // 1
    TOKEN_NUMBER,          // 2
    TOKEN_STRING,          // 3
    TOKEN_ASSIGN,          // 4: =
    TOKEN_PLUS,            // 5: +
    TOKEN_MINUS,           // 6: -
    TOKEN_ASTERISK,        // 7: *
    TOKEN_SLASH,           // 8: /
    TOKEN_LPAREN,          // 9: (
    TOKEN_RPAREN,          // 10: )
    TOKEN_COMMA,           // 11: ,
    TOKEN_ARROW,           // 12: ->
    TOKEN_FAT_ARROW,       // 13: =>
    TOKEN_FUNC,            // 14: func
    TOKEN_RETURN,          // 15: return
    TOKEN_PRINT,           // 16: print
    TOKEN_CLASS,           // 17: class
    TOKEN_IF,              // 18: if
    TOKEN_ELSE,            // 19: else
    TOKEN_FOR,             // 20: for
    TOKEN_IN,              // 21: in
    TOKEN_END,             // 22: end
    TOKEN_IMPORT,          // 23: import
    TOKEN_UI,              // 24: ui
    TOKEN_CSS,             // 25: css
    TOKEN_REGISTER_EVENT,  // 26: register_event
    TOKEN_RANGE,           // 27: range
    TOKEN_INT,             // 28: int
    TOKEN_FLOAT,           // 29: float
    TOKEN_DOT,             // 30: .
    TOKEN_DOTS,            // 31: .. (rango)
    TOKEN_SEMICOLON,       // 32: ;
    TOKEN_GT,              // 33: >
    TOKEN_LT,              // 34: <
    TOKEN_GTE,             // 35: >=
    TOKEN_LTE,             // 36: <=
    TOKEN_EQ,              // 37: ==
    TOKEN_NEQ,             // 38: !=
    TOKEN_UNKNOWN,         // 39: Caracteres no reconocidos
    TOKEN_LBRACKET,        // 40: [
    TOKEN_RBRACKET,        // 41: ]
    TOKEN_COLON,           // 42: :
    TOKEN_MODULE,          // 43: module
    TOKEN_EXPORT,          // 44: export
    TOKEN_LBRACE,          // 45: {
    TOKEN_RBRACE,          // 46: }
    TOKEN_INVALID,         // 47: Invalid token
    // New token types for control structures
    TOKEN_WHILE,           // 48: while
    TOKEN_DO,              // 49: do
    TOKEN_SWITCH,          // 50: switch
    TOKEN_CASE,            // 51: case
    TOKEN_DEFAULT,         // 52: default
    TOKEN_BREAK,           // 53: break
    TOKEN_TRY,             // 54: try
    TOKEN_CATCH,           // 55: catch
    TOKEN_FINALLY,         // 56: finally
    TOKEN_THROW,           // 57: throw
    TOKEN_MATCH,           // 58: match
    TOKEN_WHEN,            // 59: when
    TOKEN_OTHERWISE,       // 60: otherwise
    TOKEN_COMPOSE,         // 61: >> (function composition)
    TOKEN_MACRO,           // 62: macro
    TOKEN_EXPAND,          // 63: expand
    TOKEN_CONCAT,          // 64: ## (macro concatenation)
    TOKEN_STRINGIFY,       // 65: # (macro stringification)
    TOKEN_ASPECT,          // 66: aspect
    TOKEN_POINTCUT,        // 67: pointcut
    TOKEN_ADVICE,          // 68: advice
    TOKEN_BEFORE,          // 69: before
    TOKEN_AFTER,           // 70: after
    TOKEN_AROUND           // 71: around
} TokenType;

/**
 * Estructura que representa un token.
 */
typedef struct {
    TokenType type;         ///< Tipo del token.
    char lexeme[256];       ///< Cadena del token.
    int line;               ///< Línea donde aparece.
    int col;                ///< Columna donde aparece.
    union {
        char string[256];  // Para valores string
        double number;     // Para valores numéricos
    } value;
} Token;

/**
 * Estado del lexer.
 */
typedef struct {
    const char *source;     ///< Fuente de texto.
    int position;           ///< Posición actual en la fuente.
    int line;               ///< Línea actual.
    int col;                ///< Columna actual.
} LexerState;

/* Funciones públicas del lexer */
void lexerInit(const char *source);
Token getNextToken(void);
LexerState lexSaveState(void);
void lexRestoreState(LexerState state);

// Nuevas funciones para depuración
void lexer_set_debug_level(int level);  // Configura nivel de detalle de logs
const char* tokenTypeToString(TokenType type);  // Convierte tipo de token a string

#endif /* LEXER_H */
