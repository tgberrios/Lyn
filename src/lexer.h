/**
 * @file lexer.h
 * @brief Header file for the lexical analyzer of the Lyn programming language
 * 
 * This header defines the token types, token structure, and lexer interface
 * for the Lyn programming language. It includes all necessary types and
 * functions for tokenizing source code.
 */

#ifndef LYN_LEXER_H
#define LYN_LEXER_H

#include "error.h"
#include "logger.h"

/**
 * @brief Enumeration of all possible token types in the Lyn language
 * 
 * This enumeration defines all the different types of tokens that can be
 * recognized by the lexer, including keywords, operators, literals, and
 * special symbols.
 */
typedef enum {
    TOKEN_EOF = 0,         ///< End of file marker
    TOKEN_IDENTIFIER,      ///< Variable or function name
    TOKEN_NUMBER,          ///< Numeric literal
    TOKEN_STRING,          ///< String literal
    TOKEN_ASSIGN,          ///< Assignment operator (=)
    TOKEN_PLUS,            ///< Addition operator (+)
    TOKEN_MINUS,           ///< Subtraction operator (-)
    TOKEN_ASTERISK,        ///< Multiplication operator (*)
    TOKEN_SLASH,           ///< Division operator (/)
    TOKEN_LPAREN,          ///< Left parenthesis
    TOKEN_RPAREN,          ///< Right parenthesis
    TOKEN_COMMA,           ///< Comma separator
    TOKEN_ARROW,           ///< Arrow operator (->)
    TOKEN_FAT_ARROW,       ///< Fat arrow operator (=>)
    TOKEN_FUNC,            ///< Function declaration keyword
    TOKEN_RETURN,          ///< Return statement keyword
    TOKEN_PRINT,           ///< Print function keyword
    TOKEN_CLASS,           ///< Class declaration keyword
    TOKEN_IF,              ///< If statement keyword
    TOKEN_ELSE,            ///< Else statement keyword
    TOKEN_FOR,             ///< For loop keyword
    TOKEN_IN,              ///< In operator keyword
    TOKEN_END,             ///< End block keyword
    TOKEN_IMPORT,          ///< Import statement keyword
    TOKEN_FROM,            ///< 'from' keyword in import statement
    TOKEN_AS,              ///< 'as' keyword in import statement
    TOKEN_UI,              ///< UI declaration keyword
    TOKEN_CSS,             ///< CSS declaration keyword
    TOKEN_REGISTER_EVENT,  ///< Event registration keyword
    TOKEN_RANGE,           ///< Range operator keyword
    TOKEN_INT,             ///< Integer type keyword
    TOKEN_FLOAT,           ///< Float type keyword
    TOKEN_DOT,             ///< Dot operator (.)
    TOKEN_DOTS,            ///< Range operator (..)
    TOKEN_SEMICOLON,       ///< Statement separator (;)
    TOKEN_GT,              ///< Greater than operator (>)
    TOKEN_LT,              ///< Less than operator (<)
    TOKEN_GTE,             ///< Greater than or equal operator (>=)
    TOKEN_LTE,             ///< Less than or equal operator (<=)
    TOKEN_EQ,              ///< Equality operator (==)
    TOKEN_NEQ,             ///< Not equal operator (!=)
    TOKEN_UNKNOWN,         ///< Unrecognized character
    TOKEN_LBRACKET,        ///< Left bracket ([)
    TOKEN_RBRACKET,        ///< Right bracket (])
    TOKEN_TRUE,            ///< Boolean true literal
    TOKEN_FALSE,           ///< Boolean false literal
    TOKEN_AND,             ///< Logical AND operator
    TOKEN_OR,              ///< Logical OR operator
    TOKEN_COLON,           ///< Colon operator (:)
    TOKEN_MODULE,          ///< Module declaration keyword
    TOKEN_EXPORT,          ///< Export statement keyword
    TOKEN_LBRACE,          ///< Left brace ({)
    TOKEN_RBRACE,          ///< Right brace (})
    TOKEN_INVALID,         ///< Invalid token marker
    // Control structure tokens
    TOKEN_WHILE,           ///< While loop keyword
    TOKEN_DO,              ///< Do-while loop keyword
    TOKEN_SWITCH,          ///< Switch statement keyword
    TOKEN_CASE,            ///< Case statement keyword
    TOKEN_DEFAULT,         ///< Default case keyword
    TOKEN_BREAK,           ///< Break statement keyword
    TOKEN_TRY,             ///< Try block keyword
    TOKEN_CATCH,           ///< Catch block keyword
    TOKEN_FINALLY,         ///< Finally block keyword
    TOKEN_THROW,           ///< Throw statement keyword
    TOKEN_MATCH,           ///< Pattern matching keyword
    TOKEN_WHEN,            ///< Pattern matching case keyword
    TOKEN_OTHERWISE,       ///< Pattern matching default keyword
    TOKEN_COMPOSE,         ///< Function composition operator (>>)
    TOKEN_MACRO,           ///< Macro definition keyword
    TOKEN_EXPAND,          ///< Macro expansion keyword
    TOKEN_CONCAT,          ///< Macro concatenation operator (##)
    TOKEN_STRINGIFY,       ///< Macro stringification operator (#)
    TOKEN_ASPECT,          ///< Aspect declaration keyword
    TOKEN_POINTCUT,        ///< Pointcut declaration keyword
    TOKEN_ADVICE,          ///< Advice declaration keyword
    TOKEN_BEFORE,          ///< Before advice keyword
    TOKEN_AFTER,           ///< After advice keyword
    TOKEN_AROUND,          ///< Around advice keyword
    TOKEN_NEW,             ///< Object instantiation keyword
    TOKEN_THIS             ///< Current object reference
} TokenType;

/**
 * @brief Structure representing a token in the source code
 * 
 * This structure holds all the information about a token, including its type,
 * the actual text (lexeme), its location in the source code, and its value
 * if it's a literal.
 */
typedef struct {
    TokenType type;         ///< Type of the token
    char lexeme[256];       ///< The actual text of the token
    int line;               ///< Line number in source file
    int col;                ///< Column number in source file
    union {
        char string[256];   ///< String value for string literals
        double number;      ///< Numeric value for number literals
    } value;                ///< Token value (if applicable)
} Token;

/**
 * @brief Structure representing the state of the lexer
 * 
 * This structure allows saving and restoring the lexer's state, which is
 * useful for backtracking or error recovery.
 */
typedef struct {
    const char *source;     ///< Source code being processed
    int position;           ///< Current position in source
    int line;               ///< Current line number
    int col;                ///< Current column number
} LexerState;

/**
 * @brief Initializes the lexer with source code to process
 * 
 * @param source The source code string to tokenize
 */
void lexerInit(const char *source);

/**
 * @brief Initializes the lexer's internal state and keyword table
 */
void lexerInitialize(void);

/**
 * @brief Gets the next token from the source code
 * 
 * @return Token The next token in the source code
 */
Token getNextToken(void);

/**
 * @brief Saves the current state of the lexer
 * 
 * @return LexerState The current state of the lexer
 */
LexerState lexSaveState(void);

/**
 * @brief Restores the lexer to a previously saved state
 * 
 * @param state The state to restore
 */
void lexRestoreState(LexerState state);

/**
 * @brief Sets the debug level for the lexer
 * 
 * @param level The new debug level (0=none, 1=basic, 2=detailed, 3=all)
 */
void lexer_set_debug_level(int level);

/**
 * @brief Converts a token type to its string representation
 * 
 * @param type The token type to convert
 * @return const char* String representation of the token type
 */
const char* tokenTypeToString(TokenType type);

#endif /* LYN_LEXER_H */
