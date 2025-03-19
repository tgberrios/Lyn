/* parser.c */
#include "parser.h"
#include "lexer.h"
#include "memory.h"   // Usamos memory_realloc y memory_free para la gestión de memoria.
#include "error.h"    // Para usar error_report() y error_print_current()
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Estadísticas del parser
static struct {
    int nodes_created;
    int errors_found;
} parser_stats = {0, 0};

// Nivel de depuración
static int debug_level = 1;

/* Variable global para el token actual */
static Token currentToken;

void parser_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parser_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Parser debug level set to %d", level);
}

int parser_get_debug_level(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parser_get_debug_level);
    return debug_level;
}

void parser_get_stats(int* nodes_created, int* errors_found) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parser_get_stats);
    
    if (nodes_created) *nodes_created = parser_stats.nodes_created;
    if (errors_found) *errors_found = parser_stats.errors_found;
}

/* Prototipos internos */
static void advanceToken(void);
static void parserError(const char *message, Token current);
static int isLambdaLookahead(void);
static AstNode *parsePostfix(AstNode *node);
static AstNode *parseStatement(void);
static AstNode *parseExpression(void);
static AstNode *parseTerm(void);
static AstNode *parseFactor(void);
static AstNode *parseFuncDef(void);
static AstNode *parseReturn(void);
static AstNode *parseIfStmt(void);
static AstNode *parseForStmt(void);
static AstNode *parseClassDef(void);
static AstNode *parseLambda(void);
static AstNode *parseArrayLiteral(void);
static void skipStatementSeparators(void);
static AstNode* parseModuleDecl(void);
static AstNode* parseImport(void);
static AstNode *parseWhileStmt(void);
static AstNode *parseDoWhileStmt(void);
static AstNode *parseSwitchStmt(void);
static AstNode *parseBreakStmt(void);
static AstNode *parseTryCatchStmt(void);
static AstNode *parseThrowStmt(void);

/* Avanza al siguiente token */
static void advanceToken(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)advanceToken);
    
    currentToken = getNextToken();
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Token: type=%d, lexeme='%s', line=%d, col=%d",
                  currentToken.type, currentToken.lexeme, currentToken.line, currentToken.col);
    }
}

/* Reporta un error de parseo */
static void parserError(const char *message, Token current) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parserError);
    
    parser_stats.errors_found++;
    
    // Crear un mensaje de error detallado
    char detailed_msg[512];
    snprintf(detailed_msg, sizeof(detailed_msg), "%s (got '%s')", 
            message, current.lexeme);
    
    // Reportar el error usando el sistema centralizado
    error_report("parser", current.line, current.col, detailed_msg, ERROR_SYNTAX);
    
    // Imprimir el error con contexto
    error_print_current();
    
    logger_log(LOG_ERROR, "Syntax error at line %d, col %d: %s", 
              current.line, current.col, message);
    
    exit(1);
}


/* isLambdaLookahead: Comprueba sin alterar el estado global
   si la secuencia actual corresponde a la sintaxis de una lambda:
   ( [paramName : paramType {, paramName : paramType}] ) -> returnType => ... */
static int isLambdaLookahead(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)isLambdaLookahead);
    
    LexerState saved = lexSaveState();
    Token tok1 = getNextToken(); // Primer token dentro del paréntesis.
    
    // Estado para recordar si detectamos una lambda
    int result = 0;
    
    if (tok1.type == TOKEN_RPAREN) {
        Token tok2 = getNextToken();
        if (tok2.type != TOKEN_ARROW) { lexRestoreState(saved); return 0; }
        Token tok3 = getNextToken();
        if (tok3.type != TOKEN_IDENTIFIER && tok3.type != TOKEN_INT && tok3.type != TOKEN_FLOAT) {
            lexRestoreState(saved);
            return 0;
        }
        Token tok4 = getNextToken();
        // Allow either fat arrow (=>) or opening brace ({) for block lambda
        if (tok4.type != TOKEN_FAT_ARROW && tok4.type != TOKEN_LBRACE) {
            lexRestoreState(saved);
            return 0;
        }
        result = 1;
    } else {
        if (tok1.type != TOKEN_IDENTIFIER) { lexRestoreState(saved); return 0; }
        Token tokColon = getNextToken();
        if (tokColon.type != TOKEN_COLON) { lexRestoreState(saved); return 0; }
        Token tokType = getNextToken();
        if (tokType.type != TOKEN_IDENTIFIER && tokType.type != TOKEN_INT && tokType.type != TOKEN_FLOAT) {
            lexRestoreState(saved);
            return 0;
        }
        Token tok = getNextToken();
        while (tok.type == TOKEN_COMMA) {
            Token tokParam = getNextToken();
            if (tokParam.type != TOKEN_IDENTIFIER) { lexRestoreState(saved); return 0; }
            Token tokColon2 = getNextToken();
            if (tokColon2.type != TOKEN_COLON) { lexRestoreState(saved); return 0; }
            Token tokType2 = getNextToken();
            if (tokType2.type != TOKEN_IDENTIFIER && tokType2.type != TOKEN_INT && tokType2.type != TOKEN_FLOAT) {
                lexRestoreState(saved);
                return 0;
            }
            tok = getNextToken();
        }
        if (tok.type != TOKEN_RPAREN) { lexRestoreState(saved); return 0; }
        Token tokAfterParen = getNextToken();
        if (tokAfterParen.type != TOKEN_ARROW) { lexRestoreState(saved); return 0; }
        Token tokReturnType = getNextToken();
        if (tokReturnType.type != TOKEN_IDENTIFIER && tokReturnType.type != TOKEN_INT && tokReturnType.type != TOKEN_FLOAT) {
            lexRestoreState(saved);
            return 0;
        }
        Token tokFatArrow = getNextToken();
        // Allow either fat arrow (=>) or opening brace ({) for block lambda
        if (tokFatArrow.type != TOKEN_FAT_ARROW && tokFatArrow.type != TOKEN_LBRACE) {
            lexRestoreState(saved);
            return 0;
        }
        result = 1;
    }
    
    lexRestoreState(saved);
    
    if (debug_level >= 3 && result) {
        logger_log(LOG_DEBUG, "Lambda expression detected in lookahead");
    }
    
    return result;
}

/* parsePostfix: Maneja encadenamiento de '.' y '()' */
static AstNode *parsePostfix(AstNode *node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parsePostfix);
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Parsing postfix expression starting with node type %d", node->type);
    }

    if (currentToken.type == TOKEN_DOT) {
        advanceToken(); // consume '.'
        
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected identifier after '.'", currentToken);
            
        AstNode *memberNode = createAstNode(AST_MEMBER_ACCESS);
        parser_stats.nodes_created++;
        
        memberNode->memberAccess.object = node;
        strncpy(memberNode->memberAccess.member, currentToken.lexeme, sizeof(memberNode->memberAccess.member));
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Created member access node for %s", memberNode->memberAccess.member);
        }
        
        advanceToken(); // consume identifier

        if (currentToken.type == TOKEN_LPAREN) {
            advanceToken(); // consume '('
            
            AstNode *funcCall = createAstNode(AST_FUNC_CALL);
            parser_stats.nodes_created++;
            
            strncpy(funcCall->funcCall.name, memberNode->memberAccess.member, sizeof(funcCall->funcCall.name));
            funcCall->funcCall.arguments = memory_realloc(funcCall->funcCall.arguments, sizeof(AstNode *));
            funcCall->funcCall.arguments[0] = memberNode->memberAccess.object;
            funcCall->funcCall.argCount = 1;
            
            if (debug_level >= 2) {
                logger_log(LOG_DEBUG, "Converting member access to method call: %s", funcCall->funcCall.name);
            }
            
            freeAstNode(memberNode);
            
            while (currentToken.type != TOKEN_RPAREN) {
                AstNode *arg = parseExpression();
                funcCall->funcCall.argCount++;
                funcCall->funcCall.arguments = memory_realloc(funcCall->funcCall.arguments,
                                                            funcCall->funcCall.argCount * sizeof(AstNode *));
                funcCall->funcCall.arguments[funcCall->funcCall.argCount - 1] = arg;
                
                if (currentToken.type == TOKEN_COMMA)
                    advanceToken();
                else if (currentToken.type != TOKEN_RPAREN)
                    parserError("Expected ',' or ')' in argument list", currentToken);
            }
            
            advanceToken(); // consume ')'
            node = funcCall;
        } else {
            node = memberNode;
        }
        return parsePostfix(node);
    } else if (currentToken.type == TOKEN_LPAREN && node->type == AST_IDENTIFIER) {
        advanceToken(); // consume '('
        
        AstNode *funcCall = createAstNode(AST_FUNC_CALL);
        parser_stats.nodes_created++;
        
        strncpy(funcCall->funcCall.name, node->identifier.name, sizeof(funcCall->funcCall.name));
        funcCall->funcCall.arguments = NULL;
        funcCall->funcCall.argCount = 0;
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Created function call node for %s", funcCall->funcCall.name);
        }
        
        freeAstNode(node);
        
        while (currentToken.type != TOKEN_RPAREN) {
            AstNode *arg = parseExpression();
            funcCall->funcCall.argCount++;
            funcCall->funcCall.arguments = memory_realloc(funcCall->funcCall.arguments,
                                                        funcCall->funcCall.argCount * sizeof(AstNode *));
            funcCall->funcCall.arguments[funcCall->funcCall.argCount - 1] = arg;
            
            if (currentToken.type == TOKEN_COMMA)
                advanceToken();
            else if (currentToken.type != TOKEN_RPAREN)
                parserError("Expected ',' or ')' in function call argument list", currentToken);
        }
        
        advanceToken(); // consume ')'
        node = funcCall;
        return parsePostfix(node);
    }
    
    return node;
}

/* parseProgram: Se espera que el programa inicie con "main" */
AstNode *parseProgram(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseProgram);
    
    parser_stats.nodes_created = 0;
    parser_stats.errors_found = 0;
    
    logger_log(LOG_INFO, "Starting to parse program");
    
    AstNode *programNode = createAstNode(AST_PROGRAM);
    parser_stats.nodes_created++;
    
    programNode->program.statements = NULL;
    programNode->program.statementCount = 0;
    advanceToken();  // Obtener el primer token

    if (currentToken.type != TOKEN_IDENTIFIER || strcmp(currentToken.lexeme, "main") != 0)
        parserError("Program must start with 'main'", currentToken);
    advanceToken(); // consume "main"
    if (currentToken.type == TOKEN_SEMICOLON)
        advanceToken(); // consume separador

    while (currentToken.type != TOKEN_EOF &&
           !(currentToken.type == TOKEN_END && strcmp(currentToken.lexeme, "end") == 0)) {
        AstNode *stmt = parseStatement();
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Parsed statement of type %d", stmt->type);
        }
        
        skipStatementSeparators();
        
        programNode->program.statementCount++;
        programNode->program.statements = memory_realloc(programNode->program.statements,
                                                          programNode->program.statementCount * sizeof(AstNode *));
        programNode->program.statements[programNode->program.statementCount - 1] = stmt;
    }
    if (currentToken.type == TOKEN_END)
        advanceToken(); // consume final "end"
    
    logger_log(LOG_INFO, "Program parsing complete: %d nodes created, %d statements", 
              parser_stats.nodes_created, programNode->program.statementCount);
    
    return programNode;
}

/* parseStatement: Determina el tipo de sentencia según currentToken */
static AstNode *parseStatement(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseStatement);
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Parsing statement, current token: %s", currentToken.lexeme);
    }
    
    AstNode* result = NULL;
    
    if (currentToken.type == TOKEN_FUNC) {
        result = parseFuncDef();
    } else if (currentToken.type == TOKEN_RETURN) {
        result = parseReturn();
    } else if (currentToken.type == TOKEN_PRINT) {
        advanceToken(); // consume "print"
        if (currentToken.type != TOKEN_LPAREN)
            parserError("Expected '(' after 'print'", currentToken);
        advanceToken(); // consume '('
        AstNode *expr = parseExpression();
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')' after print expression", currentToken);
        advanceToken(); // consume ')'
        AstNode *printNode = createAstNode(AST_PRINT_STMT);
        parser_stats.nodes_created++;
        printNode->printStmt.expr = expr;
        result = printNode;
    } else if (currentToken.type == TOKEN_IF) {
        result = parseIfStmt();
    } else if (currentToken.type == TOKEN_FOR) {
        result = parseForStmt();
    } else if (currentToken.type == TOKEN_WHILE) {
        result = parseWhileStmt();
    } else if (currentToken.type == TOKEN_DO) {
        result = parseDoWhileStmt();
    } else if (currentToken.type == TOKEN_SWITCH) {
        result = parseSwitchStmt();
    } else if (currentToken.type == TOKEN_BREAK) {
        result = parseBreakStmt();
    } else if (currentToken.type == TOKEN_TRY) {
        result = parseTryCatchStmt();
    } else if (currentToken.type == TOKEN_THROW) {
        result = parseThrowStmt();
    } else if (currentToken.type == TOKEN_CLASS) {
        result = parseClassDef();
    } else if (currentToken.type == TOKEN_IMPORT) {
        result = parseImport();
    } else if (currentToken.type == TOKEN_UI) {
        AstNode *uiNode = createAstNode(AST_IMPORT);
        parser_stats.nodes_created++;
        strncpy(uiNode->importStmt.moduleType, "ui", sizeof(uiNode->importStmt.moduleType));
        advanceToken(); // consume "ui"
        if (currentToken.type != TOKEN_STRING)
            parserError("Expected string after 'ui'", currentToken);
        strncpy(uiNode->importStmt.moduleName, currentToken.lexeme, sizeof(uiNode->importStmt.moduleName));
        advanceToken();
        result = uiNode;
    } else if (currentToken.type == TOKEN_CSS) {
        AstNode *cssNode = createAstNode(AST_IMPORT);
        parser_stats.nodes_created++;
        strncpy(cssNode->importStmt.moduleType, "css", sizeof(cssNode->importStmt.moduleType));
        advanceToken(); // consume "css"
        if (currentToken.type != TOKEN_STRING)
            parserError("Expected string after 'css'", currentToken);
        strncpy(cssNode->importStmt.moduleName, currentToken.lexeme, sizeof(cssNode->importStmt.moduleName));
        advanceToken();
        result = cssNode;
    } else if (currentToken.type == TOKEN_REGISTER_EVENT) {
        advanceToken(); // consume "register_event"
        if (currentToken.type != TOKEN_LPAREN)
            parserError("Expected '(' after register_event", currentToken);
        advanceToken(); // consume '('
        AstNode *regCall = createAstNode(AST_FUNC_CALL);
        parser_stats.nodes_created++;
        strncpy(regCall->funcCall.name, "register_event", sizeof(regCall->funcCall.name));
        regCall->funcCall.argCount = 0;
        regCall->funcCall.arguments = NULL;
        while (currentToken.type != TOKEN_RPAREN) {
            AstNode *arg = parseExpression();
            regCall->funcCall.argCount++;
            regCall->funcCall.arguments = memory_realloc(regCall->funcCall.arguments,
                                                       regCall->funcCall.argCount * sizeof(AstNode *));
            regCall->funcCall.arguments[regCall->funcCall.argCount - 1] = arg;
            if (currentToken.type == TOKEN_COMMA)
                advanceToken();
            else if (currentToken.type != TOKEN_RPAREN)
                parserError("Expected ',' or ')' in register_event argument list", currentToken);
        }
        advanceToken(); // consume ')'
        result = regCall;
    } else if (currentToken.type == TOKEN_MODULE) {
        result = parseModuleDecl();
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        Token temp = currentToken;
        LexerState saved = lexSaveState();
        advanceToken();
        /* Rama para declaraciones explícitas con ":" */
        if (currentToken.type == TOKEN_COLON) {
            advanceToken(); // consume ':'
            char typeBuffer[256] = "";
            if (currentToken.type == TOKEN_LBRACKET) {
                /* Se trata de un tipo arreglo, e.g. [int] */
                strncat(typeBuffer, "[", sizeof(typeBuffer) - strlen(typeBuffer) - 1);
                advanceToken(); // consume '['
                if (!(currentToken.type == TOKEN_INT || currentToken.type == TOKEN_FLOAT ||
                      (currentToken.type == TOKEN_IDENTIFIER &&
                       (strcmp(currentToken.lexeme, "int") == 0 || strcmp(currentToken.lexeme, "float") == 0))))
                    parserError("Expected type inside array declaration", currentToken);
                strncat(typeBuffer, currentToken.lexeme, sizeof(typeBuffer) - strlen(typeBuffer) - 1);
                advanceToken(); // consume el tipo
                if (currentToken.type != TOKEN_RBRACKET)
                    parserError("Expected ']' after array type", currentToken);
                strncat(typeBuffer, "]", sizeof(typeBuffer) - strlen(typeBuffer) - 1);
                advanceToken(); // consume ']'
            } else {
                if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
                    parserError("Expected type after ':' in variable declaration", currentToken);
                strncpy(typeBuffer, currentToken.lexeme, sizeof(typeBuffer));
                advanceToken(); // consume tipo
            }
            AstNode *declNode = createAstNode(AST_VAR_DECL);
            strncpy(declNode->varDecl.name, temp.lexeme, sizeof(declNode->varDecl.name));
            strncpy(declNode->varDecl.type, typeBuffer, sizeof(declNode->varDecl.type));
            if (currentToken.type == TOKEN_ASSIGN) {
                advanceToken(); // consume '='
                AstNode *init = parseExpression();
                declNode->varDecl.initializer = init;
            }
            result = declNode;
        }
        /* Fin de rama de declaración explícita */

        /* Si no se encontró ':', se trata de asignación, miembro o llamada a función */
        if (currentToken.type == TOKEN_DOT) {
            advanceToken(); // consume '.'
            if (currentToken.type != TOKEN_IDENTIFIER)
                parserError("Expected identifier after '.'", currentToken);
            AstNode *memberNode = createAstNode(AST_MEMBER_ACCESS);
            parser_stats.nodes_created++;
            memberNode->memberAccess.object = createAstNode(AST_IDENTIFIER);
            strncpy(memberNode->memberAccess.object->identifier.name, temp.lexeme,
                    sizeof(memberNode->memberAccess.object->identifier.name));
            strncpy(memberNode->memberAccess.member, currentToken.lexeme,
                    sizeof(memberNode->memberAccess.member));
            advanceToken(); // consume identifier after '.'
            if (currentToken.type == TOKEN_ASSIGN) {
                advanceToken(); // consume '='
                AstNode *value = NULL;
                LexerState saved2 = lexSaveState();
                if (currentToken.type == TOKEN_LPAREN && isLambdaLookahead()) {
                    lexRestoreState(saved2);
                    value = parseLambda();
                } else {
                    lexRestoreState(saved2);
                    value = parseExpression();
                }
                AstNode *assignNode = createAstNode(AST_VAR_ASSIGN);
                snprintf(assignNode->varAssign.name, sizeof(assignNode->varAssign.name),
                         "%s.%s", temp.lexeme, memberNode->memberAccess.member);
                assignNode->varAssign.initializer = value;
                freeAstNode(memberNode);
                result = assignNode;
            } else {
                result = parsePostfix(memberNode);
            }
        }
        if (currentToken.type == TOKEN_ASSIGN) {
            advanceToken(); // consume '='
            AstNode *value;
            LexerState saved2 = lexSaveState();
            if (currentToken.type == TOKEN_LPAREN && isLambdaLookahead()) {
                lexRestoreState(saved2);
                value = parseLambda();
            } else {
                lexRestoreState(saved2);
                value = parseExpression();
            }
            AstNode *assignNode = createAstNode(AST_VAR_ASSIGN);
            strncpy(assignNode->varAssign.name, temp.lexeme, sizeof(assignNode->varAssign.name));
            assignNode->varAssign.initializer = value;
            result = assignNode;
        }
        else if (currentToken.type == TOKEN_INT ||
                 currentToken.type == TOKEN_FLOAT ||
                 (currentToken.type == TOKEN_IDENTIFIER &&
                  (strcmp(currentToken.lexeme, "int") == 0 || strcmp(currentToken.lexeme, "float") == 0))) {
            AstNode *declNode = createAstNode(AST_VAR_DECL);
            parser_stats.nodes_created++;
            strncpy(declNode->varDecl.name, temp.lexeme, sizeof(declNode->varDecl.name));
            strncpy(declNode->varDecl.type, currentToken.lexeme, sizeof(declNode->varDecl.type));
            advanceToken(); // consume tipo
            result = declNode;
        }
        else if (currentToken.type == TOKEN_LPAREN) {
            advanceToken(); // consume '('
            AstNode *funcCall = createAstNode(AST_FUNC_CALL);
            parser_stats.nodes_created++;
            strncpy(funcCall->funcCall.name, temp.lexeme, sizeof(funcCall->funcCall.name));
            funcCall->funcCall.arguments = NULL;
            funcCall->funcCall.argCount = 0;
            while (currentToken.type != TOKEN_RPAREN) {
                AstNode *arg = parseExpression();
                funcCall->funcCall.argCount++;
                funcCall->funcCall.arguments = memory_realloc(funcCall->funcCall.arguments,
                                                              funcCall->funcCall.argCount * sizeof(AstNode *));
                funcCall->funcCall.arguments[funcCall->funcCall.argCount - 1] = arg;
                if (currentToken.type == TOKEN_COMMA)
                    advanceToken();
                else if (currentToken.type != TOKEN_RPAREN)
                    parserError("Expected ',' or ')' in function call argument list", currentToken);
            }
            advanceToken(); // consume ')'
            AstNode *callNode = parsePostfix(funcCall);
            result = callNode;
        } else {
            lexRestoreState(saved);
            result = parseExpression();
        }
    } else {
        result = parseExpression();
    }
    
    if (debug_level >= 3 && result) {
        logger_log(LOG_DEBUG, "Finished parsing statement, type: %d", result->type);
    }
    
    return result;
}

/* parseExpression: Maneja operadores '+', '-', comparaciones */
static AstNode *parseExpression(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseExpression);
    
    AstNode *node = parseTerm();
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Parsed initial term for expression");
    }
    
    while (currentToken.type == TOKEN_PLUS || currentToken.type == TOKEN_MINUS ||
           currentToken.type == TOKEN_GT || currentToken.type == TOKEN_LT ||
           currentToken.type == TOKEN_GTE || currentToken.type == TOKEN_LTE ||
           currentToken.type == TOKEN_EQ || currentToken.type == TOKEN_NEQ) {
        
        char op;
        switch (currentToken.type) {
            case TOKEN_PLUS: op = '+'; break;
            case TOKEN_MINUS: op = '-'; break;
            case TOKEN_GT: op = '>'; break;
            case TOKEN_LT: op = '<'; break;
            case TOKEN_GTE: op = 'G'; break;
            case TOKEN_LTE: op = 'L'; break;
            case TOKEN_EQ: op = 'E'; break;
            case TOKEN_NEQ: op = 'N'; break;
            default: op = currentToken.lexeme[0];
        }
        
        advanceToken();
        AstNode *right = parseTerm();
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Parsed right term, creating binary op with operator '%c'", op);
        }
        
        AstNode *binOp = createAstNode(AST_BINARY_OP);
        parser_stats.nodes_created++;
        
        binOp->binaryOp.left = node;
        binOp->binaryOp.op = op;
        binOp->binaryOp.right = right;
        node = binOp;
    }
    
    return node;
}

/* parseTerm: Maneja operadores '*' y '/' */
static AstNode *parseTerm(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseTerm);
    
    AstNode *left = parseFactor();
    
    while (currentToken.type == TOKEN_ASTERISK || currentToken.type == TOKEN_SLASH) {
        char op = currentToken.lexeme[0];
        advanceToken();
        
        AstNode *right = parseFactor();
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Creating binary operation %c", op);
        }
        
        AstNode *binOp = createAstNode(AST_BINARY_OP);
        parser_stats.nodes_created++;
        
        binOp->binaryOp.left = left;
        binOp->binaryOp.op = op;
        binOp->binaryOp.right = right;
        left = binOp;
    }
    
    return left;
}

/* parseFactor: Números, cadenas, identificadores, agrupación */
static AstNode *parseFactor(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseFactor);
    
    AstNode *node = NULL;
    
    if (currentToken.type == TOKEN_LPAREN && isLambdaLookahead()) {
        return parseLambda();
    }
    if (currentToken.type == TOKEN_NUMBER) {
        node = createAstNode(AST_NUMBER_LITERAL);
        parser_stats.nodes_created++;
        node->numberLiteral.value = atof(currentToken.lexeme);
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Created number literal: %g", node->numberLiteral.value);
        }
        
        advanceToken();
    } else if (currentToken.type == TOKEN_STRING) {
        node = createAstNode(AST_STRING_LITERAL);
        parser_stats.nodes_created++;
        strncpy(node->stringLiteral.value, currentToken.lexeme, sizeof(node->stringLiteral.value));
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Created string literal: \"%s\"", node->stringLiteral.value);
        }
        
        advanceToken();
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        node = createAstNode(AST_IDENTIFIER);
        parser_stats.nodes_created++;
        strncpy(node->identifier.name, currentToken.lexeme, sizeof(node->identifier.name));
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Created identifier: %s", node->identifier.name);
        }
        
        advanceToken();
        node = parsePostfix(node);
    } else if (currentToken.type == TOKEN_LPAREN) {
        advanceToken();
        node = parseExpression();
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')' after expression", currentToken);
        advanceToken();
    } else if (currentToken.type == TOKEN_LBRACKET) {
        node = parseArrayLiteral();
    } else {
        parserError("Unexpected token in expression", currentToken);
    }
    
    return node;
}

/* parseFuncDef: Parsea una definición de función */
static AstNode *parseFuncDef(void) {
    advanceToken(); // consume 'func'
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected function name after 'func'", currentToken);
    char funcName[256];
    strncpy(funcName, currentToken.lexeme, sizeof(funcName));
    advanceToken();
    if (currentToken.type != TOKEN_LPAREN)
        parserError("Expected '(' after function name", currentToken);
    advanceToken();
    AstNode **parameters = NULL;
    int paramCount = 0;
    while (currentToken.type != TOKEN_RPAREN) {
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected parameter name in function definition", currentToken);
        char paramName[256];
        strncpy(paramName, currentToken.lexeme, sizeof(paramName));
        AstNode *param = createAstNode(AST_IDENTIFIER);
        strncpy(param->identifier.name, currentToken.lexeme, sizeof(param->identifier.name));
        parameters = memory_realloc(parameters, (paramCount + 1) * sizeof(AstNode *));
        parameters[paramCount++] = param;
        advanceToken();
        if (currentToken.type != TOKEN_COLON)
            parserError("Expected ':' after parameter name in function definition", currentToken);
        advanceToken();
        if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
            parserError("Expected parameter type in function definition", currentToken);
        advanceToken();
        if (currentToken.type == TOKEN_COMMA)
            advanceToken();
        else if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ',' or ')' in parameter list", currentToken);
    }
    advanceToken();
    char retType[64] = "";
    if (currentToken.type == TOKEN_ARROW) {
        advanceToken();
        if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
            parserError("Expected return type after '->'", currentToken);
        strncpy(retType, currentToken.lexeme, sizeof(retType));
        advanceToken();
    }
    printf("parseFuncDef: Token after header: type=%d, lexeme='%s'\n", currentToken.type, currentToken.lexeme);
    if (currentToken.type == TOKEN_SEMICOLON) {
        advanceToken();
        printf("parseFuncDef: Separador ';' consumed\n");
    }
    AstNode **body = NULL;
    int bodyCount = 0;
    while (currentToken.type == TOKEN_SEMICOLON)
        advanceToken();
    while (currentToken.type != TOKEN_END) {
        AstNode *stmt = parseStatement();
        while (currentToken.type == TOKEN_SEMICOLON)
            advanceToken();
        body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode *));
        body[bodyCount++] = stmt;
    }
    advanceToken();
    AstNode *funcNode = createAstNode(AST_FUNC_DEF);
    strncpy(funcNode->funcDef.name, funcName, sizeof(funcNode->funcDef.name));
    funcNode->funcDef.parameters = parameters;
    funcNode->funcDef.paramCount = paramCount;
    strncpy(funcNode->funcDef.returnType, retType, sizeof(funcNode->funcDef.returnType));
    funcNode->funcDef.body = body;
    funcNode->funcDef.bodyCount = bodyCount;
    return funcNode;
}

/* parseReturn: Parsea una sentencia return */
static AstNode *parseReturn(void) {
    advanceToken();
    AstNode *expr = parseExpression();
    AstNode *retNode = createAstNode(AST_RETURN_STMT);
    retNode->returnStmt.expr = expr;
    return retNode;
}

/* parseIfStmt: Parsea una estructura if-else */
static AstNode *parseIfStmt(void) {
    advanceToken();
    AstNode *condition = parseExpression();
    skipStatementSeparators();
    AstNode **thenBranch = NULL;
    int thenCount = 0;
    while (currentToken.type != TOKEN_ELSE && currentToken.type != TOKEN_END) {
        AstNode *stmt = parseStatement();
        thenBranch = memory_realloc(thenBranch, (thenCount + 1) * sizeof(AstNode *));
        thenBranch[thenCount++] = stmt;
        skipStatementSeparators();
    }
    AstNode **elseBranch = NULL;
    int elseCount = 0;
    if (currentToken.type == TOKEN_ELSE) {
        advanceToken();
        skipStatementSeparators();
        while (currentToken.type != TOKEN_END) {
            AstNode *stmt = parseStatement();
            elseBranch = memory_realloc(elseBranch, (elseCount + 1) * sizeof(AstNode *));
            elseBranch[elseCount++] = stmt;
            skipStatementSeparators();
        }
    }
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' after if statement", currentToken);
    advanceToken();
    AstNode *ifNode = createAstNode(AST_IF_STMT);
    ifNode->ifStmt.condition = condition;
    ifNode->ifStmt.thenBranch = thenBranch;
    ifNode->ifStmt.thenCount = thenCount;
    ifNode->ifStmt.elseBranch = elseBranch;
    ifNode->ifStmt.elseCount = elseCount;
    return ifNode;
}

/* parseForStmt: for i in range(...) ... end */
static AstNode *parseForStmt(void) {
    advanceToken();
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected iterator identifier in for loop", currentToken);
    char iterator[256];
    strncpy(iterator, currentToken.lexeme, sizeof(iterator));
    advanceToken();
    if (currentToken.type != TOKEN_IN)
        parserError("Expected 'in' in for loop", currentToken);
    advanceToken();
    if (currentToken.type != TOKEN_RANGE)
        parserError("Expected 'range' in for loop", currentToken);
    advanceToken();
    if (currentToken.type != TOKEN_LPAREN)
        parserError("Expected '(' after 'range'", currentToken);
    advanceToken();
    AstNode *rangeStart = parseExpression();
    AstNode *rangeEnd = NULL;
    if (currentToken.type == TOKEN_COMMA) {
        advanceToken();
        rangeEnd = parseExpression();
    } else {
        AstNode *zeroNode = createAstNode(AST_NUMBER_LITERAL);
        zeroNode->numberLiteral.value = 0;
        rangeEnd = rangeStart;
        rangeStart = zeroNode;
    }
    if (currentToken.type != TOKEN_RPAREN)
        parserError("Expected ')' after range arguments", currentToken);
    advanceToken();
    skipStatementSeparators();
    AstNode **body = NULL;
    int bodyCount = 0;
    while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
        AstNode *stmt = parseStatement();
        body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode *));
        body[bodyCount++] = stmt;
        skipStatementSeparators();
    }
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close for loop", currentToken);
    advanceToken();
    AstNode *forNode = createAstNode(AST_FOR_STMT);
    strncpy(forNode->forStmt.iterator, iterator, sizeof(forNode->forStmt.iterator));
    forNode->forStmt.rangeStart = rangeStart;
    forNode->forStmt.rangeEnd = rangeEnd;
    forNode->forStmt.body = body;
    forNode->forStmt.bodyCount = bodyCount;
    return forNode;
}

/* parseClassDef: Parsea class <Name>; ... end */
static AstNode *parseClassDef(void) {
    advanceToken();  // consume 'class'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected class name", currentToken);
    
    AstNode *classNode = createAstNode(AST_CLASS_DEF);
    strncpy(classNode->classDef.name, currentToken.lexeme, sizeof(classNode->classDef.name));
    advanceToken();
    
    // Handle inheritance
    if (currentToken.type == TOKEN_COLON) {
        advanceToken();
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected base class name after ':'", currentToken);
        strncpy(classNode->classDef.baseClassName, currentToken.lexeme, sizeof(classNode->classDef.baseClassName));
        advanceToken();
    }

    if (currentToken.type == TOKEN_SEMICOLON)
        advanceToken();
    AstNode **members = NULL;
    int memberCount = 0;
    while (currentToken.type != TOKEN_END) {
        AstNode *stmt = parseStatement();
        while (currentToken.type == TOKEN_SEMICOLON)
            advanceToken();
        members = memory_realloc(members, (memberCount + 1) * sizeof(AstNode *));
        members[memberCount++] = stmt;
    }
    advanceToken();
    classNode->classDef.members = members;
    classNode->classDef.memberCount = memberCount;
    return classNode;
}

/* parseLambda: ( <paramName> : <paramType> [, <paramName> : <paramType> ... ] ) -> <returnType> => <bodyExpr> */
static AstNode *parseLambda(void) {
    advanceToken();
    AstNode **parameters = NULL;
    int paramCount = 0;
    while (currentToken.type != TOKEN_RPAREN) {
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected parameter name in lambda", currentToken);
        AstNode *param = createAstNode(AST_IDENTIFIER);
        strncpy(param->identifier.name, currentToken.lexeme, sizeof(param->identifier.name));
        parameters = memory_realloc(parameters, (paramCount + 1) * sizeof(AstNode *));
        parameters[paramCount++] = param;
        advanceToken();
        if (currentToken.type != TOKEN_COLON)
            parserError("Expected ':' after parameter name in lambda", currentToken);
        advanceToken();
        if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
            parserError("Expected parameter type in lambda after ':'", currentToken);
        advanceToken();
        if (currentToken.type == TOKEN_COMMA)
            advanceToken();
        else if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ',' or ')' in lambda parameter list", currentToken);
    }
    advanceToken();
    if (currentToken.type != TOKEN_ARROW)
        parserError("Expected '->' after lambda parameters", currentToken);
    advanceToken();
    char retType[64] = "";
    if (currentToken.type == TOKEN_IDENTIFIER || currentToken.type == TOKEN_INT || currentToken.type == TOKEN_FLOAT) {
        strncpy(retType, currentToken.lexeme, sizeof(retType));
        advanceToken();
    }
    if (currentToken.type != TOKEN_FAT_ARROW)
        parserError("Expected '=>' in lambda", currentToken);
    advanceToken();
    AstNode *body = parseExpression();
    AstNode *lambdaNode = createAstNode(AST_LAMBDA);
    lambdaNode->lambda.parameters = parameters;
    lambdaNode->lambda.paramCount = paramCount;
    strncpy(lambdaNode->lambda.returnType, retType, sizeof(lambdaNode->lambda.returnType));
    lambdaNode->lambda.body = body;
    return lambdaNode;
}

/* parseArrayLiteral: [ elem, elem, ... ] */
static AstNode *parseArrayLiteral(void) {
    advanceToken(); // consumir '['
    AstNode **elements = NULL;
    int count = 0;
    if (currentToken.type != TOKEN_RBRACKET) {
        while (1) {
            AstNode *element = parseExpression();
            elements = memory_realloc(elements, (count + 1) * sizeof(AstNode *));
            if (!elements)
                parserError("Error de asignación de memoria para array literal", currentToken);
            elements[count++] = element;
            if (currentToken.type == TOKEN_COMMA)
                advanceToken();
            else
                break;
        }
    }
    if (currentToken.type != TOKEN_RBRACKET)
        parserError("Se esperaba ']' al finalizar el literal de arreglo", currentToken);
    advanceToken();
    AstNode *node = createAstNode(AST_ARRAY_LITERAL);
    node->arrayLiteral.elements = elements;
    node->arrayLiteral.elementCount = count;
    return node;
}

/* Función auxiliar para consumir separadores de sentencia */
static void skipStatementSeparators(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)skipStatementSeparators);
    
    int count = 0;
    while (currentToken.type == TOKEN_SEMICOLON) {
        advanceToken();
        count++;
    }
    
    if (debug_level >= 3 && count > 0) {
        logger_log(LOG_DEBUG, "Skipped %d statement separators", count);
    }
}

/* parseModuleDecl: Procesa declaraciones de módulos */
static AstNode* parseModuleDecl(void) {
    advanceToken(); // consume 'module'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected module name", currentToken);
    
    AstNode* moduleNode = createAstNode(AST_MODULE_DECL);
    strncpy(moduleNode->moduleDecl.name, currentToken.lexeme, sizeof(moduleNode->moduleDecl.name));
    
    advanceToken(); // consume module name
    
    // Parse module body
    moduleNode->moduleDecl.declarations = NULL;
    moduleNode->moduleDecl.declarationCount = 0;
    
    while (currentToken.type != TOKEN_END) {
        AstNode* decl = parseStatement();
        moduleNode->moduleDecl.declarationCount++;
        moduleNode->moduleDecl.declarations = memory_realloc(
            moduleNode->moduleDecl.declarations,
            moduleNode->moduleDecl.declarationCount * sizeof(AstNode*)
        );
        moduleNode->moduleDecl.declarations[moduleNode->moduleDecl.declarationCount - 1] = decl;
    }
    
    advanceToken(); // consume 'end'
    return moduleNode;
}

/* parseImport: Procesa sentencias import */
static AstNode* parseImport(void) {
    advanceToken(); // consume 'import'
    
    if (currentToken.type != TOKEN_STRING && currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected module name", currentToken);
    
    AstNode* importNode = createAstNode(AST_IMPORT);
    strncpy(importNode->importStmt.moduleName, currentToken.lexeme, sizeof(importNode->importStmt.moduleName));
    
    advanceToken(); // consume module name
    return importNode;
}

/* parseWhileStmt: while condition ... end */
static AstNode *parseWhileStmt(void) {
    advanceToken(); // consume 'while'
    AstNode *condition = parseExpression();
    skipStatementSeparators();
    
    AstNode **body = NULL;
    int bodyCount = 0;
    
    while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
        AstNode *stmt = parseStatement();
        body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode *));
        body[bodyCount++] = stmt;
        skipStatementSeparators();
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close while loop", currentToken);
    advanceToken(); // consume 'end'
    
    AstNode *whileNode = createAstNode(AST_WHILE_STMT);
    whileNode->whileStmt.condition = condition;
    whileNode->whileStmt.body = body;
    whileNode->whileStmt.bodyCount = bodyCount;
    
    return whileNode;
}

/* parseDoWhileStmt: do ... while condition end */
static AstNode *parseDoWhileStmt(void) {
    advanceToken(); // consume 'do'
    skipStatementSeparators();
    
    AstNode **body = NULL;
    int bodyCount = 0;
    
    while (currentToken.type != TOKEN_WHILE && currentToken.type != TOKEN_EOF) {
        AstNode *stmt = parseStatement();
        body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode *));
        body[bodyCount++] = stmt;
        skipStatementSeparators();
    }
    
    if (currentToken.type != TOKEN_WHILE)
        parserError("Expected 'while' after do block", currentToken);
    advanceToken(); // consume 'while'
    
    AstNode *condition = parseExpression();
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close do-while loop", currentToken);
    advanceToken(); // consume 'end'
    
    AstNode *doWhileNode = createAstNode(AST_DO_WHILE_STMT);
    doWhileNode->doWhileStmt.condition = condition;
    doWhileNode->doWhileStmt.body = body;
    doWhileNode->doWhileStmt.bodyCount = bodyCount;
    
    return doWhileNode;
}

/* parseSwitchStmt: switch expression case expr ... case expr ... [default ...] end */
static AstNode *parseSwitchStmt(void) {
    advanceToken(); // consume 'switch'
    AstNode *expr = parseExpression();
    skipStatementSeparators();
    
    AstNode **cases = NULL;
    int caseCount = 0;
    
    AstNode **defaultCase = NULL;
    int defaultCaseCount = 0;
    
    while (currentToken.type == TOKEN_CASE || currentToken.type == TOKEN_DEFAULT) {
        if (currentToken.type == TOKEN_CASE) {
            advanceToken(); // consume 'case'
            AstNode *caseExpr = parseExpression();
            skipStatementSeparators();
            
            AstNode **caseBody = NULL;
            int caseBodyCount = 0;
            
            while (currentToken.type != TOKEN_CASE && 
                   currentToken.type != TOKEN_DEFAULT && 
                   currentToken.type != TOKEN_END) {
                AstNode *stmt = parseStatement();
                caseBody = memory_realloc(caseBody, (caseBodyCount + 1) * sizeof(AstNode *));
                caseBody[caseBodyCount++] = stmt;
                skipStatementSeparators();
            }
            
            AstNode *caseNode = createAstNode(AST_CASE_STMT);
            caseNode->caseStmt.expr = caseExpr;
            caseNode->caseStmt.body = caseBody;
            caseNode->caseStmt.bodyCount = caseBodyCount;
            
            cases = memory_realloc(cases, (caseCount + 1) * sizeof(AstNode *));
            cases[caseCount++] = caseNode;
        } else { // TOKEN_DEFAULT
            advanceToken(); // consume 'default'
            skipStatementSeparators();
            
            while (currentToken.type != TOKEN_CASE && 
                   currentToken.type != TOKEN_END) {
                AstNode *stmt = parseStatement();
                defaultCase = memory_realloc(defaultCase, (defaultCaseCount + 1) * sizeof(AstNode *));
                defaultCase[defaultCaseCount++] = stmt;
                skipStatementSeparators();
            }
        }
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close switch statement", currentToken);
    advanceToken(); // consume 'end'
    
    AstNode *switchNode = createAstNode(AST_SWITCH_STMT);
    switchNode->switchStmt.expr = expr;
    switchNode->switchStmt.cases = cases;
    switchNode->switchStmt.caseCount = caseCount;
    switchNode->switchStmt.defaultCase = defaultCase;
    switchNode->switchStmt.defaultCaseCount = defaultCaseCount;
    
    return switchNode;
}

/* parseBreakStmt: break */
static AstNode *parseBreakStmt(void) {
    advanceToken(); // consume 'break'
    return createAstNode(AST_BREAK_STMT);
}

/* parseTryCatchStmt: try ... catch err ... [finally ...] end */
static AstNode *parseTryCatchStmt(void) {
    advanceToken(); // consume 'try'
    skipStatementSeparators();
    
    AstNode **tryBody = NULL;
    int tryCount = 0;
    
    while (currentToken.type != TOKEN_CATCH && 
           currentToken.type != TOKEN_FINALLY && 
           currentToken.type != TOKEN_END &&
           currentToken.type != TOKEN_EOF) {
        AstNode *stmt = parseStatement();
        tryBody = memory_realloc(tryBody, (tryCount + 1) * sizeof(AstNode *));
        tryBody[tryCount++] = stmt;
        skipStatementSeparators();
    }
    
    AstNode **catchBody = NULL;
    int catchCount = 0;
    char errorVarName[256] = "";
    
    if (currentToken.type == TOKEN_CATCH) {
        advanceToken(); // consume 'catch'
        
        // Parse error variable name if provided
        if (currentToken.type == TOKEN_IDENTIFIER) {
            strncpy(errorVarName, currentToken.lexeme, sizeof(errorVarName) - 1);
            advanceToken(); // consume error variable name
        }
        
        skipStatementSeparators();
        
        while (currentToken.type != TOKEN_FINALLY && 
               currentToken.type != TOKEN_END &&
               currentToken.type != TOKEN_EOF) {
            AstNode *stmt = parseStatement();
            catchBody = memory_realloc(catchBody, (catchCount + 1) * sizeof(AstNode *));
            catchBody[catchCount++] = stmt;
            skipStatementSeparators();
        }
    }
    
    AstNode **finallyBody = NULL;
    int finallyCount = 0;
    
    if (currentToken.type == TOKEN_FINALLY) {
        advanceToken(); // consume 'finally'
        skipStatementSeparators();
        
        while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
            AstNode *stmt = parseStatement();
            finallyBody = memory_realloc(finallyBody, (finallyCount + 1) * sizeof(AstNode *));
            finallyBody[finallyCount++] = stmt;
            skipStatementSeparators();
        }
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close try-catch-finally block", currentToken);
    advanceToken(); // consume 'end'
    
    AstNode *tryCatchNode = createAstNode(AST_TRY_CATCH_STMT);
    tryCatchNode->tryCatchStmt.tryBody = tryBody;
    tryCatchNode->tryCatchStmt.tryCount = tryCount;
    tryCatchNode->tryCatchStmt.catchBody = catchBody;
    tryCatchNode->tryCatchStmt.catchCount = catchCount;
    strncpy(tryCatchNode->tryCatchStmt.errorVarName, errorVarName, sizeof(tryCatchNode->tryCatchStmt.errorVarName) - 1);
    tryCatchNode->tryCatchStmt.finallyBody = finallyBody;
    tryCatchNode->tryCatchStmt.finallyCount = finallyCount;
    
    return tryCatchNode;
}

/* parseThrowStmt: throw expression */
static AstNode *parseThrowStmt(void) {
    advanceToken(); // consume 'throw'
    AstNode *expr = parseExpression();
    
    AstNode *throwNode = createAstNode(AST_THROW_STMT);
    throwNode->throwStmt.expr = expr;
    
    return throwNode;
}
