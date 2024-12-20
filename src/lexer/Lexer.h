#include <string>
#include <iostream>
#include <vector>
#pragma once
#include <stack>






// DEFINICION DE TOKENS
enum class TokenType {
    // Keywords
    ABSTRACT, ASYNC, AWAIT, BREAK, CASE, CATCH, CLASS, CONTINUE, DEFAULT, DO, ELSE, ENUM, EXPORT, EXTENDS, FALSE, FINALLY, FOR, FUNCTION, IF, IMPLEMENTS, IMPORT, INTERFACE, MODULE, NAMESPACE, NULL_VALUE, RETURN, SEALED, STRUCT, SUPER, SWITCH, THIS, THROW, TRUE, TRY, UNDEFINED_VALUE, WHILE, YIELD,

    // Operators
    MOD,CONST,ERROR,AND, ASSIGN, ASSIGN_DIVIDE, ASSIGN_MINUS, ASSIGN_MULTIPLY, ASSIGN_PLUS, BITWISE_AND, DIVIDE, EQUAL, GREATER_EQUAL, GREATER_THAN, LESS_EQUAL, LESS_THAN, MINUS, MULTIPLY, NOT, NOT_EQUAL, OR, PLUS,

    // Punctuation
    LBRACE,BITWISE_NOT,LPAREN,ARROW,RPAREN,AT_SYMBOL, CLOSE_BRACE, CLOSE_PARENTHESIS, COLON, COMMA, DOT, OPEN_BRACE, OPEN_PARENTHESIS, SEMICOLON,

    // Literals
    RBRACE,EQUAL_EQUAL,FLOAT, IDENTIFIER, NUMBER, STRING,

    // Other
    END_OF_FILE, UNKNOWN, VAR, PRINT
};

// ESTRUCTURA DE TOKENS
struct Token
{
    TokenType type;
    std::string value;
    int line;
    int column;
    
};


// CLASE LEXER QUE LEERA EL CODIGO FUENTE Y PRODUCE TOKENS

class Lexer {
    public:
        Lexer(const std::string& source);
        Token nextToken();
        void putBack(const Token& token);


    private:
    std::string source;
    size_t currentIndex = 0;
    int line = 1;
    int column = 1;
    std::stack<Token> tokenBuffer; 

    Token tokenize();
    char peek();
    char peek(int offset);
    char advance();
    bool isAtEnd();
    void skipWhitespace();
    Token makeToken(TokenType type, std::string value, int line, int column);
    Token indentifierOrKeyword();
    Token numberToken();
    Token stringToken();
    Token operatorOrSymbol();

};
