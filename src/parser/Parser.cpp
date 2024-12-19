// Parser.cpp
#include "Parser.h"
#include <iostream>
#include <stdexcept>
#include <cctype> // isspace, isalpha, isdigit, etc.
#include <iostream> // Debuging purposes
#include "lexer/Lexer.h"

// Constructor
Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance();
}

// Avanza al siguiente token
void Parser::advance() {
    currentToken = lexer.nextToken();
}

// Verifica si el currentToken coincide con el tipo especificado y lo consume si es así
bool Parser::match(TokenType type) {
    if (currentToken.type == type) {
        advance();
        return true;
    }
    return false;
}

// Espera que el currentToken sea del tipo especificado, de lo contrario lanza una excepción con el mensaje de error
void Parser::expect(TokenType type, const std::string& errorMessage) {
    if (!match(type)) {
        throw std::runtime_error(errorMessage + " at line " + std::to_string(currentToken.line) +
                                 ", column " + std::to_string(currentToken.column));
    }
}

// Verifica sin consumir si el currentToken es del tipo especificado
bool Parser::check(TokenType type) {
    return currentToken.type == type;
}

// Devuelve el currentToken actual
Token Parser::peek() const {
    return currentToken;
}

// Método principal para iniciar el análisis sintáctico
std::unique_ptr<ASTNode> Parser::parseProgram() {
    auto program = std::make_unique<BlockNode>(std::vector<std::unique_ptr<ASTNode>>());
    while (currentToken.type != TokenType::END_OF_FILE) {
        program->statements.emplace_back(parseStatement());
    }
    return program;
}

// Determina qué tipo de declaración o sentencia se está analizando y llama al método correspondiente
std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (match(TokenType::IF)) {
        return parseIfStatement();
    }
    else if (match(TokenType::FOR)) {
        return parseForStatement();
    }
    else if (match(TokenType::WHILE)) {
        return parseWhileStatement();
    }
    else if (match(TokenType::DO)) {
        return parseDoWhileStatement();
    }
    else if (match(TokenType::SWITCH)) {
        return parseSwitchStatement();
    }
    else if (match(TokenType::BREAK)) {
        return parseBreakStatement();
    }
    else if (match(TokenType::CONTINUE)) {
        return parseContinueStatement();
    }
    else if (match(TokenType::RETURN)) {
        return parseReturnStatement();
    }
    else if (match(TokenType::PRINT)) {
        return parsePrintStatement();
    }
    else if (match(TokenType::FUNCTION)) {
        return parseFunctionDeclaration();
    }
    else if (match(TokenType::CLASS)) {
        return parseClassDeclaration();
    }
    else if (match(TokenType::VAR) || match(TokenType::CONST)) {
        // Si es una declaración de variable, retrocedemos el token consumido por VAR o CONST
        lexer.putBack(currentToken);
        return parseVariableDeclaration();
    }
    else if (check(TokenType::IDENTIFIER)) {
        // Determinamos si es una asignación o una llamada a función
        Token identifierToken = currentToken;
        advance();
        if (match(TokenType::ASSIGN)) {
            auto value = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after assignment");
            auto variableNode = std::make_unique<VariableNode>(VariableNode::Type::IDENTIFIER, identifierToken.value, identifierToken.line, identifierToken.column);
            return std::make_unique<AssignmentNode>(AssignmentNode::Type::ASSIGN, std::move(variableNode), std::move(value), identifierToken.line, identifierToken.column);
        }
        else if (match(TokenType::LPAREN)) {
            // Es una llamada a función
            lexer.putBack(currentToken); // Pone de nuevo el token '('
            lexer.putBack(identifierToken); // Pone de nuevo el identificador
            return parseFunctionCall();
        }
        else {
            // Es una expresión
            lexer.putBack(currentToken); // Pone de nuevo el token no esperado
            lexer.putBack(identifierToken); // Pone de nuevo el identificador
            return parseExpressionStatement();
        }
    }
    else {
        throw std::runtime_error("Invalid statement at line " + std::to_string(currentToken.line) +
                                 ", column " + std::to_string(currentToken.column));
    }
}

// Analiza declaraciones de variables, manejando tanto variables dinámicas como constantes
std::unique_ptr<ASTNode> Parser::parseVariableDeclaration() {
    VariableNode::Type varType = VariableNode::Type::IDENTIFIER;
    if (currentToken.type == TokenType::CONST) {
        varType = VariableNode::Type::CONSTANT;
        advance(); // Consume 'const'
    }
    else {
        varType = VariableNode::Type::IDENTIFIER;
        advance(); // Consume 'var'
    }

    expect(TokenType::IDENTIFIER, "Expected variable name after 'var' or 'const'");
    std::string varName = currentToken.value;
    int varLine = currentToken.line;
    int varColumn = currentToken.column;
    advance(); // Consume identifier

    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }

    // Soporta múltiples declaraciones de variables separadas por comas
    std::vector<std::unique_ptr<ASTNode>> variables;
    auto variableNode = std::make_unique<VariableNode>(varType, varName, varLine, varColumn);
    if (initializer) {
        auto assignmentNode = std::make_unique<AssignmentNode>(AssignmentNode::Type::ASSIGN, std::move(variableNode), std::move(initializer), varLine, varColumn);
        variables.emplace_back(std::move(assignmentNode));
    }
    else {
        variables.emplace_back(std::move(variableNode));
    }

    while (match(TokenType::COMMA)) {
        expect(TokenType::IDENTIFIER, "Expected variable name after ','");
        std::string name = currentToken.value;
        int nameLine = currentToken.line;
        int nameColumn = currentToken.column;
        advance(); // Consume identifier

        std::unique_ptr<ASTNode> init = nullptr;
        if (match(TokenType::ASSIGN)) {
            init = parseExpression();
        }

        auto varNode = std::make_unique<VariableNode>(varType, name, nameLine, nameColumn);
        if (init) {
            auto assignNode = std::make_unique<AssignmentNode>(AssignmentNode::Type::ASSIGN, std::move(varNode), std::move(init), nameLine, nameColumn);
            variables.emplace_back(std::move(assignNode));
        }
        else {
            variables.emplace_back(std::move(varNode));
        }
    }

    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    return std::make_unique<VariableDeclarationNode>(std::move(variables));
}

// Analiza una asignación a una variable existente
std::unique_ptr<ASTNode> Parser::parseAssignment() {
    expect(TokenType::IDENTIFIER, "Expected variable name for assignment");
    std::string varName = currentToken.value;
    int varLine = currentToken.line;
    int varColumn = currentToken.column;
    advance(); // Consume identifier

    expect(TokenType::ASSIGN, "Expected '=' after variable name");

    auto value = parseExpression();

    expect(TokenType::SEMICOLON, "Expected ';' after assignment");

    auto variableNode = std::make_unique<VariableNode>(VariableNode::Type::IDENTIFIER, varName, varLine, varColumn);
    return std::make_unique<AssignmentNode>(AssignmentNode::Type::ASSIGN, std::move(variableNode), std::move(value), varLine, varColumn);
}

// Analiza una expresión aritmética y lógica respetando la precedencia de los operadores
std::unique_ptr<ASTNode> Parser::parseExpression() {
    return parseLogicalOr();
}

// Implementa niveles de precedencia para operadores lógicos y de comparación
std::unique_ptr<ASTNode> Parser::parseLogicalOr() {
    auto node = parseLogicalAnd();
    while (match(TokenType::OR)) { // Supongamos que tienes un token OR para '||'
        TokenType operatorToken = TokenType::OR;
        auto right = parseLogicalAnd();
        node = std::make_unique<BinaryOperationNode>(operatorToken, std::move(node), std::move(right), currentToken.line, currentToken.column);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseLogicalAnd() {
    auto node = parseEquality();
    while (match(TokenType::AND)) { // Supongamos que tienes un token AND para '&&'
        TokenType operatorToken = TokenType::AND;
        auto right = parseEquality();
        node = std::make_unique<BinaryOperationNode>(operatorToken, std::move(node), std::move(right), currentToken.line, currentToken.column);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseEquality() {
    auto node = parseComparison();
    while (currentToken.type == TokenType::EQUAL_EQUAL || currentToken.type == TokenType::NOT_EQUAL) {
        TokenType operatorToken = currentToken.type;
        int opLine = currentToken.line;
        int opColumn = currentToken.column;
        advance(); // Consume '==' o '!='
        auto right = parseComparison();
        node = std::make_unique<BinaryOperationNode>(operatorToken, std::move(node), std::move(right), opLine, opColumn);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseComparison() {
    auto node = parseTerm();
    while (currentToken.type == TokenType::LESS_THAN || currentToken.type == TokenType::LESS_EQUAL ||
           currentToken.type == TokenType::GREATER_THAN || currentToken.type == TokenType::GREATER_EQUAL) {
        TokenType operatorToken = currentToken.type;
        int opLine = currentToken.line;
        int opColumn = currentToken.column;
        advance(); // Consume comparison operator
        auto right = parseTerm();
        node = std::make_unique<BinaryOperationNode>(operatorToken, std::move(node), std::move(right), opLine, opColumn);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (currentToken.type == TokenType::MULTIPLY || currentToken.type == TokenType::DIVIDE ||
           currentToken.type == TokenType::MOD) {
        TokenType operatorToken = currentToken.type;
        int opLine = currentToken.line;
        int opColumn = currentToken.column;
        advance(); // Consume '*', '/', or '%'
        auto right = parseFactor();
        node = std::make_unique<BinaryOperationNode>(operatorToken, std::move(node), std::move(right), opLine, opColumn);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parseFactor() {
    if (match(TokenType::NOT)) {
        int opLine = currentToken.line;
        int opColumn = currentToken.column;
        expect(TokenType::LPAREN, "Expected '(' after '!'");
        auto operand = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return std::make_unique<UnaryOperationNode>(TokenType::NOT, std::move(operand), opLine, opColumn);
    }
    else if (match(TokenType::BITWISE_NOT)) {
        int opLine = currentToken.line;
        int opColumn = currentToken.column;
        expect(TokenType::LPAREN, "Expected '(' after '~'");
        auto operand = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return std::make_unique<UnaryOperationNode>(TokenType::BITWISE_NOT, std::move(operand), opLine, opColumn);
    }
    else if (match(TokenType::LPAREN)) {
        auto expression = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expression;
    }
    else if (check(TokenType::IDENTIFIER)) {
        return parseVariable();
    }
    else if (check(TokenType::NUMBER) || check(TokenType::STRING) ||
             check(TokenType::TRUE) || check(TokenType::FALSE) || check(TokenType::NULL_VALUE)) {
        return parseLiteral();
    }
    else {
        throw std::runtime_error("Invalid factor at line " + std::to_string(currentToken.line) +
                                 ", column " + std::to_string(currentToken.column));
    }
}

// Analiza un bloque de código encerrado entre { y }
std::unique_ptr<ASTNode> Parser::parseBlock() { 
    expect(TokenType::LBRACE, "Expected '{' to start a block");
    auto block = std::make_unique<BlockNode>(std::vector<std::unique_ptr<ASTNode>>());
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        block->statements.emplace_back(parseStatement());
    }
    expect(TokenType::RBRACE, "Expected '}' after block");
    return block;         
}

// Analiza una estructura if-else
std::unique_ptr<ASTNode> Parser::parseIfStatement() {
    expect(TokenType::IF, "Expected 'if'");
    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    auto thenBranch = parseStatement();
    std::unique_ptr<ASTNode> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = parseStatement();
    }
    return std::make_unique<IfNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch), currentToken.line, currentToken.column);
}

// Analiza una estructura for
std::unique_ptr<ASTNode> Parser::parseForStatement() {
    expect(TokenType::FOR, "Expected 'for'");
    expect(TokenType::LPAREN, "Expected '(' after 'for'");
    
    // Inicialización
    std::unique_ptr<ASTNode> initializer;
    if (match(TokenType::VAR) || match(TokenType::CONST)) {
        // Retrocedemos el token VAR o CONST para que parseVariableDeclaration lo maneje
        lexer.putBack(currentToken);
        initializer = parseVariableDeclaration();
    }
    else if (!check(TokenType::SEMICOLON)) {
        initializer = parseExpressionStatement();
    }
    else {
        expect(TokenType::SEMICOLON, "Expected ';' after initializer in 'for' loop");
    }
    
    // Condición
    std::unique_ptr<ASTNode> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after condition in 'for' loop");
    
    // Incremento
    std::unique_ptr<ASTNode> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parseExpression();
    }
    expect(TokenType::RPAREN, "Expected ')' after increment in 'for' loop");
    
    // Cuerpo del for
    auto body = parseStatement();
    
    return std::make_unique<ForNode>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), currentToken.line, currentToken.column);
}

// Analiza una estructura while
std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
    expect(TokenType::WHILE, "Expected 'while'");
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    auto body = parseStatement();
    return std::make_unique<WhileNode>(std::move(condition), std::move(body), currentToken.line, currentToken.column);
}

// Analiza una estructura do-while
std::unique_ptr<ASTNode> Parser::parseDoWhileStatement() {
    expect(TokenType::DO, "Expected 'do'");
    auto body = parseStatement();
    expect(TokenType::WHILE, "Expected 'while' after 'do' body");
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    expect(TokenType::SEMICOLON, "Expected ';' after 'do-while' statement");
    return std::make_unique<DoWhileNode>(std::move(body), std::move(condition), currentToken.line, currentToken.column);
}

// Analiza una estructura switch
std::unique_ptr<ASTNode> Parser::parseSwitchStatement() {
    expect(TokenType::SWITCH, "Expected 'switch'");
    expect(TokenType::LPAREN, "Expected '(' after 'switch'");
    auto expression = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after switch expression");
    auto body = parseBlock(); // El cuerpo contendrá CaseNode y DefaultNode
    return std::make_unique<SwitchNode>(std::move(expression), std::move(body), currentToken.line, currentToken.column);
}

// Analiza una cláusula case dentro de un switch
std::unique_ptr<ASTNode> Parser::parseCaseStatement() {
    expect(TokenType::CASE, "Expected 'case'");
    auto condition = parseExpression();
    expect(TokenType::COLON, "Expected ':' after case condition");
    auto body = parseStatement();
    return std::make_unique<CaseNode>(std::move(condition), std::move(body), currentToken.line, currentToken.column);
}

// Analiza una cláusula default dentro de un switch
std::unique_ptr<ASTNode> Parser::parseDefaultStatement() {
    expect(TokenType::DEFAULT, "Expected 'default'");
    expect(TokenType::COLON, "Expected ':' after 'default'");
    auto body = parseStatement();
    return std::make_unique<DefaultNode>(std::move(body), currentToken.line, currentToken.column);
}

// Analiza una instrucción break
std::unique_ptr<ASTNode> Parser::parseBreakStatement() {
    expect(TokenType::BREAK, "Expected 'break'");
    expect(TokenType::SEMICOLON, "Expected ';' after 'break'");
    return std::make_unique<BreakNode>(currentToken.line, currentToken.column);
}

// Analiza una instrucción continue
std::unique_ptr<ASTNode> Parser::parseContinueStatement() {
    expect(TokenType::CONTINUE, "Expected 'continue'");
    expect(TokenType::SEMICOLON, "Expected ';' after 'continue'");
    return std::make_unique<ContinueNode>(currentToken.line, currentToken.column);
}

// Analiza una instrucción return
std::unique_ptr<ASTNode> Parser::parseReturnStatement() {
    expect(TokenType::RETURN, "Expected 'return'");
    std::unique_ptr<ASTNode> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after 'return' statement");
    return std::make_unique<ReturnNode>(std::move(value), currentToken.line, currentToken.column);
}

// Analiza una instrucción print
std::unique_ptr<ASTNode> Parser::parsePrintStatement() {
    expect(TokenType::PRINT, "Expected 'print'");
    std::unique_ptr<ASTNode> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after 'print' statement");
    return std::make_unique<PrintNode>(std::move(value), currentToken.line, currentToken.column);
}

// Analiza la declaración de una función
std::unique_ptr<ASTNode> Parser::parseFunctionDeclaration() {
    expect(TokenType::FUNCTION, "Expected 'function'");
    expect(TokenType::IDENTIFIER, "Expected function name after 'function'");
    std::string funcName = currentToken.value;
    int funcLine = currentToken.line;
    int funcColumn = currentToken.column;
    advance(); // Consume function name

    expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<ASTNode>> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            auto param = parseVariable();
            parameters.emplace_back(std::move(param));
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after function parameters");

    // Opcional: Manejar el tipo de retorno
    if (match(TokenType::ARROW)) {
        // Implementa parseType() si tu lenguaje soporta tipado
        throw std::runtime_error("Return type parsing not implemented yet");
    }

    auto body = parseBlock();
    return std::make_unique<FunctionDeclarationNode>(funcName, std::move(parameters), std::move(body), funcLine, funcColumn);
}

// Analiza una llamada a una función
std::unique_ptr<ASTNode> Parser::parseFunctionCall() {
    expect(TokenType::IDENTIFIER, "Expected function name for call");
    std::string funcName = currentToken.value;
    int funcLine = currentToken.line;
    int funcColumn = currentToken.column;
    advance(); // Consume function name

    expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<ASTNode>> arguments;
    if (!check(TokenType::RPAREN)) {
        do {
            auto arg = parseExpression();
            arguments.emplace_back(std::move(arg));
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after function arguments");
    expect(TokenType::SEMICOLON, "Expected ';' after function call");

    return std::make_unique<FunctionCallNode>(funcName, std::move(arguments), funcLine, funcColumn);
}

// Analiza la declaración de una clase
std::unique_ptr<ASTNode> Parser::parseClassDeclaration() {    
    expect(TokenType::CLASS, "Expected 'class'");
    expect(TokenType::IDENTIFIER, "Expected class name after 'class'");
    std::string className = currentToken.value;
    int classLine = currentToken.line;
    int classColumn = currentToken.column;
    advance(); // Consume class name

    std::string baseClass = "";
    if (match(TokenType::EXTENDS)) {
        expect(TokenType::IDENTIFIER, "Expected base class name after 'extends'");
        baseClass = currentToken.value;
        advance(); // Consume base class name
    }

    auto body = parseBlock();
    return std::make_unique<ClassDeclarationNode>(className, baseClass, std::move(body), classLine, classColumn);
}

// Analiza un método dentro de una clase
std::unique_ptr<ASTNode> Parser::parseClassMethod() {
    expect(TokenType::FUNCTION, "Expected 'function' for class method");
    expect(TokenType::IDENTIFIER, "Expected method name after 'function'");
    std::string methodName = currentToken.value;
    int methodLine = currentToken.line;
    int methodColumn = currentToken.column;
    advance(); // Consume method name

    expect(TokenType::LPAREN, "Expected '(' after method name");
    std::vector<std::unique_ptr<ASTNode>> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            auto param = parseVariable();
            parameters.emplace_back(std::move(param));
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after method parameters");

    // Opcional: Manejar el tipo de retorno
    if (match(TokenType::ARROW)) {
        // Implementa parseType() si tu lenguaje soporta tipado
        throw std::runtime_error("Return type parsing not implemented yet");
    }

    auto body = parseBlock();
    return std::make_unique<ClassMethodNode>(methodName, std::move(parameters), std::move(body), methodLine, methodColumn);
}

// Analiza una propiedad dentro de una clase
std::unique_ptr<ASTNode> Parser::parseClassProperty() {
    expect(TokenType::IDENTIFIER, "Expected property name in class");
    std::string propertyName = currentToken.value;
    int propLine = currentToken.line;
    int propColumn = currentToken.column;
    advance(); // Consume property name

    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }

    expect(TokenType::SEMICOLON, "Expected ';' after class property");
    return std::make_unique<ClassPropertyNode>(propertyName, std::move(initializer), propLine, propColumn);
}

// Analiza una expresión que se usa como sentencia, como una llamada a función sin asignación
std::unique_ptr<ASTNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExpressionStatementNode>(std::move(expr), currentToken.line, currentToken.column);
}

// Analiza un literal (número, cadena, booleano, null)
std::unique_ptr<ASTNode> Parser::parseLiteral() {
    if (match(TokenType::NUMBER)) {
        std::string value = currentToken.value;
        int litLine = currentToken.line;
        int litColumn = currentToken.column;
        advance(); // Consume number
        // Determina si es entero o flotante
        if (value.find('.') != std::string::npos) {
            return std::make_unique<LiteralNode>(LiteralNode::Type::FLOAT, value, litLine, litColumn);
        }
        else {
            return std::make_unique<LiteralNode>(LiteralNode::Type::INT, value, litLine, litColumn);
        }
    }
    else if (match(TokenType::STRING)) {
        std::string value = currentToken.value;
        int litLine = currentToken.line;
        int litColumn = currentToken.column;
        advance(); // Consume string
        return std::make_unique<LiteralNode>(LiteralNode::Type::STRING, value, litLine, litColumn);
    }
    else if (match(TokenType::TRUE)) {
        int litLine = currentToken.line;
        int litColumn = currentToken.column;
        advance(); // Consume 'true'
        return std::make_unique<LiteralNode>(LiteralNode::Type::BOOLEAN, "true", litLine, litColumn);
    }
    else if (match(TokenType::FALSE)) {
        int litLine = currentToken.line;
        int litColumn = currentToken.column;
        advance(); // Consume 'false'
        return std::make_unique<LiteralNode>(LiteralNode::Type::BOOLEAN, "false", litLine, litColumn);
    }
    else if (match(TokenType::NULL_VALUE)) {
        int litLine = currentToken.line;
        int litColumn = currentToken.column;
        advance(); // Consume 'null'
        return std::make_unique<LiteralNode>(LiteralNode::Type::NULL_VALUE, "null", litLine, litColumn);
    }
    else {
        throw std::runtime_error("Invalid literal at line " + std::to_string(currentToken.line) +
                                 ", column " + std::to_string(currentToken.column));
    }
}
