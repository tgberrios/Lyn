/**
 * @file lexer.c
 * @brief Implementation of the lexical analyzer for the Lyn programming language
 * 
 * This file implements the lexical analyzer (lexer) that converts source code
 * into a stream of tokens. It handles keywords, identifiers, numbers, strings,
 * and various operators and punctuation marks.
 */

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
    #define DBG_PRINT(...) /* Do nothing */
#endif

#define TOKEN_INVALID -1  ///< Invalid token type

///< Current source code being processed
static const char *source;
static int position;      ///< Current position in source code
static int line = 1;      ///< Current line number
static int col = 1;       ///< Current column number
static int debug_level = 1; ///< Current debug level

#define KEYWORD_TABLE_SIZE 101  ///< Size of the keyword hash table

/**
 * @brief Structure for keyword lookup table
 */
static struct {
    char* keyword;       ///< The keyword string
    TokenType type;      ///< The corresponding token type
} keyword_table[KEYWORD_TABLE_SIZE];
static int keyword_count = 0;  ///< Number of keywords in the table

/**
 * @brief Inserts a keyword into the lookup table
 * 
 * @param word The keyword string to insert
 * @param type The token type associated with the keyword
 */
static void insertKeyword(const char* word, TokenType type) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)insertKeyword);
    if (keyword_count >= KEYWORD_TABLE_SIZE) {
        logger_log(LOG_ERROR, "Keyword table is full, cannot add more keywords");
        return;
    }
    keyword_table[keyword_count].keyword = memory_strdup(word);
    keyword_table[keyword_count].type = type;
    keyword_count++;
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Added keyword '%s' with token type %d", word, type);
    }
}

/**
 * @brief Looks up a word in the keyword table
 * 
 * @param word The word to look up
 * @return TokenType The token type if found, TOKEN_IDENTIFIER otherwise
 */
static TokenType lookupKeyword(const char* word) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lookupKeyword);
    for (int i = 0; i < keyword_count; i++) {
        if (strcmp(keyword_table[i].keyword, word) == 0) {
            return keyword_table[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

/**
 * @brief Initializes the keyword lookup table with all language keywords
 */
static void initializeKeywords(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)initializeKeywords);
    // Add keywords
    insertKeyword("func", TOKEN_FUNC);
    insertKeyword("return", TOKEN_RETURN);
    insertKeyword("print", TOKEN_PRINT);
    insertKeyword("class", TOKEN_CLASS);
    insertKeyword("if", TOKEN_IF);
    insertKeyword("else", TOKEN_ELSE);
    insertKeyword("for", TOKEN_FOR);
    insertKeyword("in", TOKEN_IN);
    insertKeyword("end", TOKEN_END);
    insertKeyword("import", TOKEN_IMPORT);
    insertKeyword("ui", TOKEN_UI);
    insertKeyword("css", TOKEN_CSS);
    insertKeyword("register_event", TOKEN_REGISTER_EVENT);
    insertKeyword("range", TOKEN_RANGE);
    insertKeyword("int", TOKEN_INT);
    insertKeyword("float", TOKEN_FLOAT);
    insertKeyword("module", TOKEN_MODULE);
    insertKeyword("export", TOKEN_EXPORT);
    insertKeyword("while", TOKEN_WHILE);
    insertKeyword("do", TOKEN_DO);
    insertKeyword("switch", TOKEN_SWITCH);
    insertKeyword("case", TOKEN_CASE);
    insertKeyword("default", TOKEN_DEFAULT);
    insertKeyword("break", TOKEN_BREAK);
    insertKeyword("try", TOKEN_TRY);
    insertKeyword("catch", TOKEN_CATCH);
    insertKeyword("finally", TOKEN_FINALLY);
    insertKeyword("throw", TOKEN_THROW);
    insertKeyword("match", TOKEN_MATCH);
    insertKeyword("when", TOKEN_WHEN);
    insertKeyword("otherwise", TOKEN_OTHERWISE);
    insertKeyword("aspect", TOKEN_ASPECT);
    insertKeyword("pointcut", TOKEN_POINTCUT);
    insertKeyword("advice", TOKEN_ADVICE);
    insertKeyword("before", TOKEN_BEFORE);
    insertKeyword("after", TOKEN_AFTER);
    insertKeyword("around", TOKEN_AROUND);
    insertKeyword("true", TOKEN_TRUE);
    insertKeyword("false", TOKEN_FALSE);
    insertKeyword("and", TOKEN_AND);
    insertKeyword("or", TOKEN_OR);
    insertKeyword("new", TOKEN_NEW);
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Initialized %d keywords", keyword_count);
    }
}

/**
 * @brief Initializes the lexer and keyword table
 */
void lexerInitialize(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexerInitialize);
    for (int i = 0; i < keyword_count; i++) {
        memory_free(keyword_table[i].keyword);
    }
    keyword_count = 0;
    initializeKeywords();
    logger_log(LOG_INFO, "Lexer initialized with %d keywords", keyword_count);
}

/**
 * @brief Initializes the lexer with source code
 * 
 * @param src The source code to tokenize
 */
void lexerInit(const char *src) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexerInit);
    logger_log(LOG_INFO, "Initializing lexer");
    source = src;
    position = 0;
    line = 1;
    col = 1;
    error_set_source(src);
}

/**
 * @brief Reports a lexer error and exits
 * 
 * @param message The error message to report
 */
static void lexerError(const char* message) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)lexerError);
    logger_log(LOG_ERROR, "Lexer error: %s at line %d, col %d", message, line, col);
    error_report("lexer", line, col, message, ERROR_SYNTAX);
    error_print_current();
    exit(1);
}

/**
 * @brief Saves the current state of the lexer
 * 
 * @return LexerState The current state of the lexer
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
 * @brief Restores the lexer to a previously saved state
 * 
 * @param state The state to restore
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
 * @brief Advances the lexer position and returns the current character
 * 
 * @return char The current character
 */
static char advance(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)advance);
    col++;
    return source[position++];
}

/**
 * @brief Peeks at the next character without advancing
 * 
 * @return char The next character
 */
static char peek(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)peek);
    return source[position];
}

/**
 * @brief Skips whitespace and comments in the source code
 */
static void skipWhitespaceAndComments(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)skipWhitespaceAndComments);
    int oldLine = line, oldCol = col, oldPos = position;
    while (1) {
        while (isspace(source[position])) {
            if (source[position] == '\n') {
                line++;
                col = 0;
            }
            advance();
        }
        if (source[position] == '/' && source[position + 1] == '/') {
            while (source[position] != '\n' && source[position] != '\0')
                advance();
            continue;
        }
        if (source[position] == '/' && source[position + 1] == '*') {
            advance(); advance();
            while (!(source[position] == '*' && source[position + 1] == '/') && source[position] != '\0') {
                if (source[position] == '\n') { line++; col = 0; }
                advance();
            }
            if (source[position] != '\0') { advance(); advance(); }
            continue;
        }
        break;
    }
    if (debug_level >= 3 && oldLine != line) {
        logger_log(LOG_DEBUG, "Skipped from line %d, col %d to line %d, col %d", oldLine, oldCol, line, col);
    }
}

/**
 * @brief Gets the next token from the source code
 * 
 * @return Token The next token in the source code
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
    
    // Handle identifiers and keywords
    if (isalpha(c) || c == '_') {
        int start = position - 1;
        while (isalnum(source[position]) || source[position] == '_')
            advance();
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';
        token.type = lookupKeyword(token.lexeme);
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: %s '%s' at line %d, col %d", 
                      tokenTypeToString(token.type), token.lexeme, token.line, token.col);
        }
        return token;
    }
    
    // Handle numbers
    if (isdigit(c) || (c == '.' && isdigit(peek()))) {
        int start = position - 1;
        while (isdigit(source[position]) || source[position] == '.')
            advance();
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';
        token.type = TOKEN_NUMBER;
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
    
    // Handle string literals
    if (c == '"') {
        int start = position;
        while (source[position] != '"' && source[position] != '\0') {
            if (source[position] == '\n')
                lexerError("Unterminated string literal");
            advance();
        }
        if (source[position] == '\0')
            lexerError("Unterminated string literal");
        int length = position - start;
        strncpy(token.lexeme, source + start, length);
        token.lexeme[length] = '\0';
        advance(); // Consume closing quote
        token.type = TOKEN_STRING;
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Lexer produced token: %s \"%s\" at line %d, col %d", 
                      tokenTypeToString(token.type), token.lexeme, token.line, token.col);
        }
        return token;
    }
    
    // Handle operators and punctuation
    switch (c) {
        case '=':
            if (peek() == '=') { advance(); token.type = TOKEN_EQ; strcpy(token.lexeme, "=="); }
            else if (peek() == '>') { advance(); token.type = TOKEN_FAT_ARROW; strcpy(token.lexeme, "=>"); }
            else { token.type = TOKEN_ASSIGN; token.lexeme[0] = '='; token.lexeme[1] = '\0'; }
            break;
        case ':':
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
            if (peek() == '>') { advance(); token.type = TOKEN_ARROW; strcpy(token.lexeme, "->"); }
            else { token.type = TOKEN_MINUS; token.lexeme[0] = '-'; token.lexeme[1] = '\0'; }
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
            if (peek() == '=') { advance(); token.type = TOKEN_GTE; strcpy(token.lexeme, ">="); }
            else if (peek() == '>') { advance(); token.type = TOKEN_COMPOSE; strcpy(token.lexeme, ">>"); }
            else { token.type = TOKEN_GT; token.lexeme[0] = '>'; token.lexeme[1] = '\0'; }
            break;
        case '<':
            if (peek() == '=') { advance(); token.type = TOKEN_LTE; strcpy(token.lexeme, "<="); }
            else { token.type = TOKEN_LT; token.lexeme[0] = '<'; token.lexeme[1] = '\0'; }
            break;
        case '!':
            if (peek() == '=') { advance(); token.type = TOKEN_NEQ; strcpy(token.lexeme, "!="); }
            else { token.type = TOKEN_UNKNOWN; snprintf(token.lexeme, sizeof(token.lexeme), "Unknown character"); }
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            token.lexeme[0] = '[';
            token.lexeme[1] = '\0';
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            token.lexeme[0] = ']';
            token.lexeme[1] = '\0';
            break;
        case '{':
            token.type = TOKEN_LBRACE;
            strcpy(token.lexeme, "{");
            break;
        case '}':
            token.type = TOKEN_RBRACE;
            strcpy(token.lexeme, "}");
            break;
        default:
            token.type = TOKEN_UNKNOWN;
            snprintf(token.lexeme, sizeof(token.lexeme), "Unknown character '%c'", c);
            logger_log(LOG_WARNING, "Unknown character '%c' (%d) at line %d, col %d", c, (int)c, line, col-1);
            break;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Lexer produced token: %s '%s' at line %d, col %d", 
                  tokenTypeToString(token.type), token.lexeme, token.line, token.col);
    }
    return token;
}

/**
 * @brief Converts a token type to its string representation
 * 
 * @param type The token type to convert
 * @return const char* String representation of the token type
 */
const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_THIS: return "TOKEN_THIS";
        default: return "TOKEN_UNKNOWN";
    }
}

/**
 * @brief Sets the debug level for the lexer
 * 
 * @param level The new debug level (0=none, 1=basic, 2=detailed, 3=all)
 */
void lexer_set_debug_level(int level) {
    debug_level = level;
    logger_log(LOG_INFO, "Lexer debug level set to %d", level);
}
