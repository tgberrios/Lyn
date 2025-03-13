#ifndef LYN_LEXER_H
#define LYN_LEXER_H

// Añadir la inclusión del sistema de errores
#include "error.h"

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
    TOKEN_SEMICOLON,       // 31: ;
    TOKEN_GT,              // 32: >
    TOKEN_LT,              // 33: <
    TOKEN_GTE,             // 34: >=
    TOKEN_LTE,             // 35: <=
    TOKEN_EQ,              // 36: ==
    TOKEN_NEQ,             // 37: !=
    TOKEN_UNKNOWN,         // 38: Caracteres no reconocidos
    TOKEN_LBRACKET,        // 39: [
    TOKEN_RBRACKET,        // 40: ]
    TOKEN_COLON,           // 41: :
    TOKEN_MODULE,          // 42: module
    TOKEN_EXPORT,          // 43: export
    TOKEN_LBRACE,          // 44: {
    TOKEN_RBRACE,          // 45: }
    TOKEN_INVALID = -1     // Add this line
} TokenType;

/**
 * Estructura que representa un token.
 */
typedef struct {
    TokenType type;         ///< Tipo del token.
    char lexeme[256];       ///< Cadena del token.
    int line;               ///< Línea donde aparece.
    int col;                ///< Columna donde aparece.
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

#endif /* LEXER_H */
