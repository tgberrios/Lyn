// Parser.h
#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"    // Asegúrate de que la ruta es correcta
#include "AST.h"
#include <memory>
#include <stdexcept>
#include <vector>

class Parser {
public:
    // Constructor que recibe una referencia al lexer
    Parser(Lexer& lexer);
    
    // Método principal para iniciar el análisis sintáctico
    std::unique_ptr<ASTNode> parseProgram();

private:
    Lexer& lexer;
    Token currentToken;

    // Métodos auxiliares
    void advance();
    bool match(TokenType type);
    void expect(TokenType type, const std::string& errorMessage);
    bool check(TokenType type);
    Token peek() const;

    // Métodos de análisis según la gramática
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseVariableDeclaration();
    std::unique_ptr<ASTNode> parseAssignment();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseExpressionStatement(); // Método faltante
    std::unique_ptr<ASTNode> parseLogicalOr();            // Método faltante
    std::unique_ptr<ASTNode> parseLogicalAnd();           // Método faltante
    std::unique_ptr<ASTNode> parseEquality();             // Método faltante
    std::unique_ptr<ASTNode> parseComparison();           // Método faltante
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parseUnary();                // Método faltante
    std::unique_ptr<ASTNode> parseVariable();             // Método faltante
    std::unique_ptr<ASTNode> parseLiteral();              // Método faltante
    std::unique_ptr<ASTNode> parseBlock();
    
    // Estructuras de control
    std::unique_ptr<ASTNode> parseIfStatement();
    std::unique_ptr<ASTNode> parseForStatement();
    std::unique_ptr<ASTNode> parseWhileStatement();
    std::unique_ptr<ASTNode> parseDoWhileStatement();
    std::unique_ptr<ASTNode> parseSwitchStatement();
    std::unique_ptr<ASTNode> parseCaseStatement();
    std::unique_ptr<ASTNode> parseDefaultStatement();
    
    // Funciones y clases
    std::unique_ptr<ASTNode> parseFunctionDeclaration();
    std::unique_ptr<ASTNode> parseFunctionCall();
    std::unique_ptr<ASTNode> parseClassDeclaration();
    std::unique_ptr<ASTNode> parseClassMethod();
    std::unique_ptr<ASTNode> parseClassProperty();
    
    // Instrucciones
    std::unique_ptr<ASTNode> parseBreakStatement();
    std::unique_ptr<ASTNode> parseContinueStatement();
    std::unique_ptr<ASTNode> parseReturnStatement();
    
    // Otros
    std::unique_ptr<ASTNode> parsePrintStatement();
};

#endif // PARSER_H
