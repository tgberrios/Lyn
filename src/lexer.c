#include "lexer.h"
#include "memory.h"
#include "error.h"
#include "logger.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_MEMORY
    #define DBG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
    #define DBG_PRINT(...) /* No hace nada */
#endif

#define TOKEN_INVALID -1  // Add this at the top of the file with the other token definitions

static const char *source;
static int position;
static int line = 1;
static int col = 1;

// Nivel de depuración (0=mínimo, 3=máximo)
static int debug_level = 1;

/**
 * @brief Inicializa el lexer con la fuente de entrada.
 *
 * @param src Puntero a la cadena de entrada.
 */
void lexerInit(const char *src) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexerInit);
    logger_log(LOG_INFO, "Initializing lexer");
    
    source = src;
    position = 0;
    line = 1;
    col = 1;
    error_set_source(src);  // Configura el código fuente para el sistema de errores
}

static void lexerError(const char* message) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexerError);
    logger_log(LOG_ERROR, "Lexer error: %s at line %d, col %d", message, line, col);
    
    error_report("lexer", line, col, message, ERROR_SYNTAX);
    error_print_current();
    exit(1);
}

/**
 * @brief Guarda el estado actual del lexer.
 *
 * @return LexerState Estado actual del lexer.
 */
LexerState lexSaveState(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexSaveState);
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Saving lexer state at line %d, col %d, pos %d", line, col, position);
    }
    
    LexerState state = { source, position, line, col };
    return state;
}

/**
 * @brief Restaura el estado del lexer.
 *
 * @param state Estado a restaurar.
 */
void lexRestoreState(LexerState state) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexRestoreState);
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Restoring lexer state to line %d, col %d, pos %d", 
                  state.line, state.col, state.position);
    }
    
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
    error_push_debug(__func__, __FILE__, __LINE__, (void*)advance);
    col++;
    return source[position++];
}

/**
 * @brief Retorna el carácter actual sin avanzar la posición.
 *
 * @return char Carácter actual.
 */
static char peek(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)peek);
    return source[position];
}

/**
 * @brief Omite espacios en blanco, saltos de línea y comentarios.
 *
 * Salta todos los espacios, saltos de línea y comentarios (línea y bloque)
 * para posicionar el lexer en el siguiente token relevante.
 */
static void skipWhitespaceAndComments(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)skipWhitespaceAndComments);
    
    int oldLine = line;
    int oldCol = col;
    int oldPos = position;
    
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
    
    // Registramos los saltos de línea encontrados para depuración avanzada
    if (debug_level >= 3 && oldLine != line) {
        logger_log(LOG_DEBUG, "Skipped from line %d, col %d to line %d, col %d", 
                  oldLine, oldCol, line, col);
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
    error_push_debug(__func__, __FILE__, __LINE__, (void*)getNextToken);
    
    skipWhitespaceAndComments();

    if (source[position] == '\0') {
        Token token = { TOKEN_EOF, "EOF", line, col };
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: EOF at line %d, col %d", line, col);
        }
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

        // Verificación de palabras clave (código existente)
        // ...existing code for keyword checks...
        
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
        // New keywords for control structures
        else if (strcmp(token.lexeme, "while") == 0) token.type = TOKEN_WHILE;
        else if (strcmp(token.lexeme, "do") == 0) token.type = TOKEN_DO;
        else if (strcmp(token.lexeme, "switch") == 0) token.type = TOKEN_SWITCH;
        else if (strcmp(token.lexeme, "case") == 0) token.type = TOKEN_CASE;
        else if (strcmp(token.lexeme, "default") == 0) token.type = TOKEN_DEFAULT;
        else if (strcmp(token.lexeme, "break") == 0) token.type = TOKEN_BREAK;
        else if (strcmp(token.lexeme, "try") == 0) token.type = TOKEN_TRY;
        else if (strcmp(token.lexeme, "catch") == 0) token.type = TOKEN_CATCH;
        else if (strcmp(token.lexeme, "finally") == 0) token.type = TOKEN_FINALLY;
        else if (strcmp(token.lexeme, "throw") == 0) token.type = TOKEN_THROW;
        else token.type = TOKEN_IDENTIFIER;
        
        // Log del token reconocido
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: %s '%s' at line %d, col %d", 
                      tokenTypeToString(token.type), token.lexeme, token.line, token.col);
        }
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
        
        // Validar formato de número para detectar errores comunes como '1.2.3'
        int dotCount = 0;
        for (int i = 0; i < length; i++) {
            if (token.lexeme[i] == '.') dotCount++;
        }
        
        if (dotCount > 1) {
            lexerError("Invalid number format - multiple decimal points");
        }
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: %s '%s' at line %d, col %d", 
                      tokenTypeToString(token.type), token.lexeme, token.line, token.col);
        }
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
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: %s \"%s\" at line %d, col %d", 
                      tokenTypeToString(token.type), token.lexeme, token.line, token.col);
        }
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
        case '{':   // Add this case
            token.type = TOKEN_LBRACE;
            strcpy(token.lexeme, "{");
            break;
        case '}':   // Add this case
            token.type = TOKEN_RBRACE;
            strcpy(token.lexeme, "}");
            break;
        default:
            token.type = TOKEN_UNKNOWN;
            snprintf(token.lexeme, sizeof(token.lexeme), "Unknown character '%c'", c);
            logger_log(LOG_WARNING, "Unknown character '%c' (%d) at line %d, col %d", 
                      c, (int)c, line, col-1);
            break;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Lexer produced token: %s '%s' at line %d, col %d", 
                  tokenTypeToString(token.type), token.lexeme, token.line, token.col);
    }
    return token;
}

// Add TOKEN_LBRACE and TOKEN_RBRACE if they don't exist
static TokenType charToToken(char c) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)charToToken);
    
    switch (c) {
        case '(': return TOKEN_LPAREN;
        case ')': return TOKEN_RPAREN;
        case '[': return TOKEN_LBRACKET;
        case ']': return TOKEN_RBRACKET;
        case '{': return TOKEN_LBRACE;   // Add this if missing
        case '}': return TOKEN_RBRACE;   // Add this if missing
        case '.': return TOKEN_DOT;
        case ',': return TOKEN_COMMA;
        case ';': return TOKEN_SEMICOLON;
        case ':': return TOKEN_COLON;
        case '+': return TOKEN_PLUS;
        case '-': return TOKEN_MINUS;
        case '*': return TOKEN_ASTERISK;
        case '/': return TOKEN_SLASH;
        case '=': return TOKEN_ASSIGN;
        case '<': return TOKEN_LT;
        case '>': return TOKEN_GT;
        default: return TOKEN_INVALID;  // Then fix the charToToken function to use TOKEN_INVALID
    }
}

// Convertir tipo de token a cadena para depuración
const char* tokenTypeToString(TokenType type) {
    static const char* tokenNames[] = {
        "TOKEN_EOF", "TOKEN_IDENTIFIER", "TOKEN_NUMBER", "TOKEN_STRING",
        "TOKEN_ASSIGN", "TOKEN_PLUS", "TOKEN_MINUS", "TOKEN_ASTERISK", 
        "TOKEN_SLASH", "TOKEN_LPAREN", "TOKEN_RPAREN", "TOKEN_COMMA", 
        "TOKEN_ARROW", "TOKEN_FAT_ARROW", "TOKEN_FUNC", "TOKEN_RETURN",
        "TOKEN_PRINT", "TOKEN_CLASS", "TOKEN_IF", "TOKEN_ELSE",
        "TOKEN_FOR", "TOKEN_IN", "TOKEN_END", "TOKEN_IMPORT",
        "TOKEN_UI", "TOKEN_CSS", "TOKEN_REGISTER_EVENT", "TOKEN_RANGE",
        "TOKEN_INT", "TOKEN_FLOAT", "TOKEN_DOT", "TOKEN_SEMICOLON",
        "TOKEN_GT", "TOKEN_LT", "TOKEN_GTE", "TOKEN_LTE",
        "TOKEN_EQ", "TOKEN_NEQ", "TOKEN_UNKNOWN", "TOKEN_LBRACKET",
        "TOKEN_RBRACKET", "TOKEN_COLON", "TOKEN_MODULE", "TOKEN_EXPORT",
        "TOKEN_LBRACE", "TOKEN_RBRACE", "TOKEN_INVALID", "TOKEN_WHILE",
        "TOKEN_DO", "TOKEN_SWITCH", "TOKEN_CASE", "TOKEN_DEFAULT",
        "TOKEN_BREAK", "TOKEN_TRY", "TOKEN_CATCH", "TOKEN_FINALLY",
        "TOKEN_THROW"
    };
    
    if (type >= 0 && type <= TOKEN_THROW) {
        return tokenNames[type];
    }
    return "UNKNOWN_TOKEN_TYPE";
}

void lexer_set_debug_level(int level) {
    debug_level = level;
    logger_log(LOG_INFO, "Lexer debug level set to %d", level);
}
