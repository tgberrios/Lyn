#include "Lexer.h"
#include <cctype> // isspace, isalpha, isdigit, etc.
#include <iostream> // Debuging purposes

// CONSTRUCTOR
Lexer::Lexer(const std::string& source) : source(source) {}



// PEAK() DEVUELVE EL CARACTER ACTUAL SIN CONSUMIRLO
char Lexer::peek() {
  return isAtEnd() ? '\0' : source[currentIndex];
}

// PEEK(OFFSET) PERMITE VER MAS ADELANTE SIN MOVER EL CURRENTINDEX
char Lexer::peek(int offset) {
  return (currentIndex + offset < source.size()) ? source[currentIndex + offset] : '\0';
}

// ADVANCE() CONSUME EL CARACTER ACTUAL Y MOVE EL CURRENTINDEX
char Lexer ::advance() {
  char current = source[currentIndex++];
  if (current == '\n') {
    line++;
    column = 1; }
  else {
    column++;
  }
  return current;
}

// ISATEND() RETORNA TRUE SI EL CURRENTINDEX ES EL FIN DEL ARRAY
bool Lexer::isAtEnd() {
  return currentIndex >= source.size();
};

Token Lexer::nextToken() {
  //VARIALBLES
  char c = peek();
  
  
  // Saltar espacios en blanco
  skipWhitespace();

  //2. Si estamos al final del codigo, devolver END_OF_FILE
  if (isAtEnd()) {
    return makeToken(TokenType::END_OF_FILE, "", line, column);
  }

  //3. Si empieza con letra o '_', es un IDENTIFIER
  if (isalpha(c) || c == '_') { 
    return indentifierOrKeyword();
  }

  //4. Si es un numero, es un NUMBER
  if (std::isdigit(c)) { 
    return numberToken();
  }

  //5. Si es un string, es un STRING
  if (c == '"') {
    return stringToken();
  }

  // Si no es ninguno de los anteriores, es un  operador o simbolo especial
  return operatorOrSymbol();

}


// HELPER FUNCTIONS
void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char current = peek();
    if (isspace(current) || current == '\n') {
      advance();
      continue;
    }
    else {
      break;
    }
  }
}

Token Lexer::makeToken(TokenType type, std::string value, int line, int column) {
    Token t;
    t.type = type;
    t.value = value;
    t.line = line;
    t.column = column;
    return t;
}

Token Lexer::indentifierOrKeyword() {
  int starLine = line;
  int StarColumn = column;

  std::string value;

  while (std::isalnum((unsigned char)peek()) || peek() == '_') {
    value += advance(); }

   // COMPARAR CON PALABRAS CLAVES
   if ( value == "var") return makeToken(TokenType::VAR, value, line, column);
   if ( value == "if") return makeToken(TokenType::IF, value, line, column);
   if ( value == "else") return makeToken(TokenType::ELSE, value, line, column);
   if ( value == "print") return makeToken(TokenType::PRINT, value, line, column);

   return makeToken(TokenType::IDENTIFIER, value, line, column);
}

Token Lexer::numberToken() {
std::string value;
while (std::isdigit((unsigned char) peek())) { 
  value += advance();
}
return makeToken(TokenType::NUMBER, value, line, column);
}

Token Lexer::stringToken() { 
  advance(); // Consumir '"'
  
  std::string value; // Guardar el string
  
  // Mientras no sea el final del código y el siguiente carácter no sea '"'
  while (!isAtEnd() && peek() != '"') { 
    char c = advance();
    value += c;
  }

  // Si llegamos al final del código sin encontrar un '"', devolver un token de error
  if (isAtEnd()) {
    return makeToken(TokenType::ERROR, "Unterminated string", line, column);
  } 

  // Consumir el carácter final '"'
  advance();

  // Devolver el token STRING
  return makeToken(TokenType::STRING, value, line, column);
}

Token Lexer::operatorOrSymbol() {
  char current = peek();
  switch (current) {
    case '+':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::ASSIGN_PLUS, "+=", line, column);
      }
      return makeToken(TokenType::PLUS, "+", line, column);
    case '-':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::ASSIGN_MINUS, "-=", line, column);
      }
      return makeToken(TokenType::MINUS, "-", line, column);
    case '*':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::ASSIGN_MULTIPLY, "*=", line, column);
      }
      return makeToken(TokenType::MULTIPLY, "*", line, column);
    case '/':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::ASSIGN_DIVIDE, "/=", line, column);
      }
      return makeToken(TokenType::DIVIDE, "/", line, column);
    case '=':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::EQUAL, "==", line, column);
      }
      return makeToken(TokenType::ASSIGN, "=", line, column);
    case '!':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::NOT_EQUAL, "!=", line, column);
      }
      return makeToken(TokenType::NOT, "!", line, column);
    case '<':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::LESS_EQUAL, "<=", line, column);
      }
      return makeToken(TokenType::LESS_THAN, "<", line, column);
    case '>':
      advance();
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::GREATER_EQUAL, ">=", line, column);
      }
      return makeToken(TokenType::GREATER_THAN, ">", line, column);
    case '&':
      advance();
      if (peek() == '&') {
        advance();
        return makeToken(TokenType::AND, "&&", line, column);
      }
      return makeToken(TokenType::BITWISE_AND, "&", line, column);
    case '|':
      advance();
      if (peek() == '|') {
        advance();
        return makeToken(TokenType::OR, "||", line, column);
      }
      return makeToken(TokenType::UNKNOWN, "|", line, column);
    case '(':
      advance();
      return makeToken(TokenType::OPEN_PARENTHESIS, "(", line, column);
    case ')':
      advance();
      return makeToken(TokenType::CLOSE_PARENTHESIS, ")", line, column);
    case '{':
      advance();
      return makeToken(TokenType::OPEN_BRACE, "{", line, column);
    case '}':
      advance();
      return makeToken(TokenType::CLOSE_BRACE, "}", line, column);
    case ',':
      advance();
      return makeToken(TokenType::COMMA, ",", line, column);
    case '.':
      advance();
      return makeToken(TokenType::DOT, ".", line, column);
    case ':':
      advance();
      return makeToken(TokenType::COLON, ":", line, column);
    case ';':
      advance();
      return makeToken(TokenType::SEMICOLON, ";", line, column);
    case '@':
      advance();
      return makeToken(TokenType::AT_SYMBOL, "@", line, column);
    default:
      advance();
      return makeToken(TokenType::UNKNOWN, std::string(1, current), line, column);
  }
}





