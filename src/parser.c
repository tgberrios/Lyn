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
static AstNode* parseCurryExpression(AstNode* baseFunc);
static AstNode* parsePatternMatch(void);
static AstNode* parseFunctionComposition(void);

// Nuevos prototipos para AspectJ (sin cambios respecto a lo anterior)
static AstNode* parseAspect(void);
static AstNode* parsePointcut(void);
static AstNode* parseAdvice(void);

// --- NUEVAS FUNCIONES PARA INSTANCIACIÓN Y THIS ---

// Función para parsear una expresión 'new Clase(arg1, arg2, ...)'
static AstNode* parseNewExpr(void) {
    // Se asume que el token 'new' ya está presente
    advanceToken(); // consume 'new'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected class name after 'new'", currentToken);
    
    AstNode *newNode = createAstNode(AST_NEW_EXPR);
    strncpy(newNode->newExpr.className, currentToken.lexeme, sizeof(newNode->newExpr.className));
    advanceToken(); // consume el nombre de la clase
    
    if (currentToken.type != TOKEN_LPAREN)
        parserError("Expected '(' after class name in new expression", currentToken);
    advanceToken(); // consume '('
    
    newNode->newExpr.arguments = NULL;
    newNode->newExpr.argCount = 0;
    
    if (currentToken.type != TOKEN_RPAREN) {
        do {
            AstNode *arg = parseExpression();
            newNode->newExpr.argCount++;
            newNode->newExpr.arguments = memory_realloc(newNode->newExpr.arguments,
                                                         newNode->newExpr.argCount * sizeof(AstNode*));
            newNode->newExpr.arguments[newNode->newExpr.argCount - 1] = arg;
            if (currentToken.type == TOKEN_COMMA)
                advanceToken();
        } while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF);
    }
    
    if (currentToken.type != TOKEN_RPAREN)
        parserError("Expected ')' after new expression arguments", currentToken);
    
    advanceToken(); // consume ')'
    return newNode;
}

// --- FIN DE NUEVAS FUNCIONES ---

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
    
    char detailed_msg[512];
    snprintf(detailed_msg, sizeof(detailed_msg), "%s (got '%s')", 
            message, current.lexeme);
    
    error_report("parser", current.line, current.col, detailed_msg, ERROR_SYNTAX);
    error_print_current();
    
    logger_log(LOG_ERROR, "Syntax error at line %d, col %d: %s", 
              current.line, current.col, message);
    
    exit(1);
}

/* isLambdaLookahead: Verifica si la secuencia corresponde a una lambda */
static int isLambdaLookahead(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)isLambdaLookahead);
    
    LexerState saved = lexSaveState();
    Token tok1 = getNextToken();
    
    int result = 0;
    
    if (tok1.type == TOKEN_RPAREN) {
        Token tok2 = getNextToken();
        if (tok2.type != TOKEN_ARROW) { lexRestoreState(saved); return 0; }
        Token tok3 = getNextToken();
        if (tok3.type != TOKEN_IDENTIFIER && tok3.type != TOKEN_INT && tok3.type != TOKEN_FLOAT) {
            if (tok3.type != TOKEN_FAT_ARROW && tok3.type != TOKEN_LBRACE) {
                lexRestoreState(saved);
                return 0;
            }
        }
        Token tok4 = getNextToken();
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
            tok = getNextToken();
            if (tok.type != TOKEN_IDENTIFIER) { lexRestoreState(saved); return 0; }
            tok = getNextToken();
            if (tok.type != TOKEN_COLON) { lexRestoreState(saved); return 0; }
            tok = getNextToken();
            if (tok.type != TOKEN_IDENTIFIER && tok.type != TOKEN_INT && tok.type != TOKEN_FLOAT) { 
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
            if (tokReturnType.type != TOKEN_FAT_ARROW && tokReturnType.type != TOKEN_LBRACE) {
                lexRestoreState(saved);
                return 0;
            }
        }
        Token tokFatArrow = getNextToken();
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

/* parsePostfix: Maneja encadenamiento de '.', '()', y '[]' */
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
            logger_log(LOG_DEBUG, "Created member access node for '%s'", memberNode->memberAccess.member);
        }
        
        advanceToken(); // consume identifier

        if (currentToken.type == TOKEN_LPAREN) {
            // Caso especial: obj.método(...) se transforma en una llamada a método
            advanceToken(); // consume '('
            
            AstNode *funcCall = createAstNode(AST_FUNC_CALL);
            parser_stats.nodes_created++;
            
            char fullMethodName[512] = "";
            const char* className = "Object";
            // Si el objeto es una instancia creada con new, usamos su nombre de clase
            if (memberNode->memberAccess.object->type == AST_NEW_EXPR) {
                className = memberNode->memberAccess.object->newExpr.className;
            } else if (memberNode->memberAccess.object->type == AST_IDENTIFIER) {
                // Opcional: si se hubiera inferido el tipo, se podría usar aquí
                className = memberNode->memberAccess.object->identifier.name;
            }
            snprintf(fullMethodName, sizeof(fullMethodName), "%s.%s", className, memberNode->memberAccess.member);
            
            strncpy(funcCall->funcCall.name, fullMethodName, sizeof(funcCall->funcCall.name) - 1);
            
            // Agrega el objeto como el primer argumento (this/self)
            funcCall->funcCall.argCount = 1;
            funcCall->funcCall.arguments = malloc(sizeof(AstNode*));
            funcCall->funcCall.arguments[0] = memberNode->memberAccess.object;
            
            // Parsear argumentos adicionales
            if (currentToken.type != TOKEN_RPAREN) {
                do {
                    if (currentToken.type == TOKEN_COMMA) advanceToken();
                    
                    AstNode *arg = parseExpression();
                    
                    funcCall->funcCall.argCount++;
                    funcCall->funcCall.arguments = realloc(funcCall->funcCall.arguments, 
                                               funcCall->funcCall.argCount * sizeof(AstNode*));
                    funcCall->funcCall.arguments[funcCall->funcCall.argCount - 1] = arg;
                } while (currentToken.type == TOKEN_COMMA);
            }
            
            if (currentToken.type != TOKEN_RPAREN)
                parserError("Expected ')'", currentToken);
            
            advanceToken(); // consume ')'
            
            freeAstNode(memberNode);
            
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
            logger_log(LOG_DEBUG, "Created function call node for '%s'", funcCall->funcCall.name);
        }
        
        freeAstNode(node);
        
        if (currentToken.type != TOKEN_RPAREN) {
            do {
                AstNode *arg = parseExpression();
                funcCall->funcCall.argCount++;
                funcCall->funcCall.arguments = memory_realloc(funcCall->funcCall.arguments,
                                                              funcCall->funcCall.argCount * sizeof(AstNode *));
                funcCall->funcCall.arguments[funcCall->funcCall.argCount - 1] = arg;
                if (currentToken.type == TOKEN_COMMA)
                    advanceToken();
                else if (currentToken.type != TOKEN_RPAREN)
                    parserError("Expected ',' or ')' in function call argument list", currentToken);
            } while (currentToken.type == TOKEN_COMMA);
        }
        
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')'", currentToken);
        
        advanceToken(); // consume ')'
        node = funcCall;
        return parsePostfix(node);
    } else if (currentToken.type == TOKEN_LBRACKET) {
        advanceToken(); // consume '['
        
        AstNode *arrayAccess = createAstNode(AST_ARRAY_ACCESS);
        parser_stats.nodes_created++;
        
        arrayAccess->arrayAccess.array = node;
        arrayAccess->arrayAccess.index = parseExpression();
        
        if (currentToken.type != TOKEN_RBRACKET)
            parserError("Expected ']'", currentToken);
        
        advanceToken(); // consume ']'
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Created array access node");
        }
        
        return parsePostfix(arrayAccess);
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

    // Parse zero or more top-level function definitions
    while (currentToken.type == TOKEN_FUNC) {
        parseFuncDef();
        // ...existing code...
    }

    // Ensure main block follows
    if (currentToken.type != TOKEN_IDENTIFIER || strcmp(currentToken.lexeme, "main") != 0) {
        parserError("Program must start with 'main'", currentToken);
    }
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
    } else if (currentToken.type == TOKEN_FROM) {
        // Manejar sintaxis "from module import symbol1, symbol2..."
        advanceToken(); // consume 'from'
        
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected module name after 'from'", currentToken);
            
        char moduleName[256];
        strncpy(moduleName, currentToken.lexeme, sizeof(moduleName) - 1);
        moduleName[sizeof(moduleName) - 1] = '\0';
        
        advanceToken(); // consume module name
        
        if (currentToken.type != TOKEN_IMPORT)
            parserError("Expected 'import' after module name in 'from' statement", currentToken);
        
        // Crear nodo de importación
        AstNode* importNode = createAstNode(AST_IMPORT);
        parser_stats.nodes_created++;
        
        // Configurar campos
        strncpy(importNode->importStmt.moduleName, moduleName, sizeof(importNode->importStmt.moduleName) - 1);
        importNode->importStmt.hasSymbolList = true;
        importNode->importStmt.symbolCount = 0;
        importNode->importStmt.symbols = NULL;
        importNode->importStmt.aliases = NULL;
        
        advanceToken(); // consume 'import'
        
        // Procesar lista de símbolos
        do {
            if (currentToken.type != TOKEN_IDENTIFIER)
                parserError("Expected identifier in import list", currentToken);
            
            // Incrementar contador de símbolos
            importNode->importStmt.symbolCount++;
            
            // Reasignar memoria para los arreglos de símbolos y alias
            importNode->importStmt.symbols = (const char**)memory_realloc(
                (void*)importNode->importStmt.symbols, 
                importNode->importStmt.symbolCount * sizeof(char*)
            );
            
            importNode->importStmt.aliases = (const char**)memory_realloc(
                (void*)importNode->importStmt.aliases, 
                importNode->importStmt.symbolCount * sizeof(char*)
            );
            
            if (!importNode->importStmt.symbols || !importNode->importStmt.aliases) {
                parserError("Memory allocation error in import statement", currentToken);
            }
            
            // Guardar nombre del símbolo
            importNode->importStmt.symbols[importNode->importStmt.symbolCount - 1] = memory_strdup(currentToken.lexeme);
            importNode->importStmt.aliases[importNode->importStmt.symbolCount - 1] = NULL; // Por defecto no hay alias
            
            advanceToken(); // consume nombre del símbolo
            
            // Verificar si hay un 'as' para alias
            if (currentToken.type == TOKEN_AS) {
                advanceToken(); // consume 'as'
                
                if (currentToken.type != TOKEN_IDENTIFIER)
                    parserError("Expected identifier after 'as' in import statement", currentToken);
                
                // Guardar el alias
                importNode->importStmt.aliases[importNode->importStmt.symbolCount - 1] = memory_strdup(currentToken.lexeme);
                
                advanceToken(); // consume el alias
            }
            
            // Si hay coma, hay más símbolos por importar
            if (currentToken.type == TOKEN_COMMA) {
                advanceToken(); // consume ','
            } else {
                break; // Final de la lista de símbolos
            }
        } while (1);
        
        result = importNode;
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
            if (regCall->funcCall.argCount > 0) {
                if (currentToken.type != TOKEN_COMMA)
                    parserError("Expected ',' between arguments", currentToken);
                advanceToken(); // consume ','
            }
            AstNode *arg = parseExpression();
            regCall->funcCall.argCount++;
            regCall->funcCall.arguments = realloc(regCall->funcCall.arguments, 
                                         regCall->funcCall.argCount * sizeof(AstNode*));
            regCall->funcCall.arguments[regCall->funcCall.argCount - 1] = arg;
        }
        advanceToken(); // consume ')'
        result = regCall;
    } else if (currentToken.type == TOKEN_MODULE) {
        result = parseModuleDecl();
    } else if (currentToken.type == TOKEN_MATCH) {
        result = parsePatternMatch();
    } else if (currentToken.type == TOKEN_ASPECT) {
        result = parseAspect();
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        Token temp = currentToken;
        LexerState saved = lexSaveState();
        advanceToken();
        
        if (currentToken.type == TOKEN_COLON) {
            advanceToken(); // consume ':'
            
            if (currentToken.type != TOKEN_IDENTIFIER &&
                currentToken.type != TOKEN_INT &&
                currentToken.type != TOKEN_FLOAT) {
                parserError("Expected type after ':' in variable declaration", currentToken);
            }
            
            char typeBuffer[256] = "";
            strncpy(typeBuffer, currentToken.lexeme, sizeof(typeBuffer));
            advanceToken(); // consume type
            
            AstNode *declNode = createAstNode(AST_VAR_DECL);
            strncpy(declNode->varDecl.name, temp.lexeme, sizeof(declNode->varDecl.name));
            strncpy(declNode->varDecl.type, typeBuffer, sizeof(declNode->varDecl.type));
            
            if (currentToken.type == TOKEN_ASSIGN) {
                advanceToken(); // consume '='
                AstNode *init = parseExpression();
                declNode->varDecl.initializer = init;
            }
            
            result = declNode;
        } else {
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
            } else if (currentToken.type == TOKEN_ASSIGN) {
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
            } else if (currentToken.type == TOKEN_INT ||
                       currentToken.type == TOKEN_FLOAT ||
                       (currentToken.type == TOKEN_IDENTIFIER &&
                        (strcmp(currentToken.lexeme, "int") == 0 || strcmp(currentToken.lexeme, "float") == 0))) {
                AstNode *declNode = createAstNode(AST_VAR_DECL);
                parser_stats.nodes_created++;
                strncpy(declNode->varDecl.name, temp.lexeme, sizeof(declNode->varDecl.name));
                strncpy(declNode->varDecl.type, currentToken.lexeme, sizeof(declNode->varDecl.type));
                advanceToken(); // consume tipo
                result = declNode;
            } else if (currentToken.type == TOKEN_LPAREN) {
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
        }
    } else {
        result = parseExpression();
    }
    
    if (debug_level >= 3 && result) {
        logger_log(LOG_DEBUG, "Finished parsing statement, type: %d", result->type);
    }
    
    return result;
}

/* parseExpression: Maneja operadores de adición, comparación y concatenación */
static AstNode *parseExpression(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseExpression);
    
    AstNode *node = parseTerm();
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Parsed initial term for expression");
    }
    
    if (currentToken.type == TOKEN_COMPOSE) {
        advanceToken(); // consume '>>'
        AstNode* rightFunc = parseTerm();
        
        AstNode* composeNode = createAstNode(AST_FUNC_COMPOSE);
        parser_stats.nodes_created++;
        
        composeNode->funcCompose.left = node;
        composeNode->funcCompose.right = rightFunc;
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Created function composition node");
        }
        
        node = composeNode;
        
        while (currentToken.type == TOKEN_COMPOSE) {
            advanceToken(); // consume '>>'
            rightFunc = parseTerm();
            
            composeNode = createAstNode(AST_FUNC_COMPOSE);
            parser_stats.nodes_created++;
            
            composeNode->funcCompose.left = node;
            composeNode->funcCompose.right = rightFunc;
            
            node = composeNode;
        }
        
        return node;
    }
    
    while (currentToken.type == TOKEN_PLUS || currentToken.type == TOKEN_MINUS ||
           currentToken.type == TOKEN_GT || currentToken.type == TOKEN_LT ||
           currentToken.type == TOKEN_GTE || currentToken.type == TOKEN_LTE ||
           currentToken.type == TOKEN_EQ || currentToken.type == TOKEN_NEQ ||
           currentToken.type == TOKEN_AND || currentToken.type == TOKEN_OR) {
        
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
            case TOKEN_AND: op = 'A'; break;
            case TOKEN_OR: op = 'O'; break;
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

/* parseFactor: Maneja números, cadenas, identificadores, agrupación, new y this */
static AstNode *parseFactor(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseFactor);
    
    AstNode *node = NULL;
    
    // Soporte para instanciación de objetos con 'new'
    if (currentToken.type == TOKEN_NEW) {
        node = parseNewExpr();
        return node;
    }
    // Soporte para 'this'
    if (currentToken.type == TOKEN_THIS) {
        AstNode *thisNode = createAstNode(AST_THIS_EXPR);
        advanceToken();
        return thisNode;
    }
    
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
        
        if (strcmp(node->identifier.name, "not") == 0) {
            AstNode* notExpr = createAstNode(AST_UNARY_OP);
            parser_stats.nodes_created++;
            notExpr->unaryOp.op = 'N';
            notExpr->unaryOp.expr = parseFactor();
            return notExpr;
        }
        
        if (currentToken.type == TOKEN_LPAREN) {
            LexerState saved = lexSaveState();
            advanceToken();
            
            AstNode *funcCall = createAstNode(AST_FUNC_CALL);
            parser_stats.nodes_created++;
            strncpy(funcCall->funcCall.name, node->identifier.name, sizeof(funcCall->funcCall.name));
            funcCall->funcCall.arguments = NULL;
            funcCall->funcCall.argCount = 0;
            
            while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
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
            
            if (currentToken.type == TOKEN_LPAREN) {
                return parseCurryExpression(funcCall);
            } else {
                freeAstNode(node);
                return funcCall;
            }
        } else {
            node = parsePostfix(node);
        }
    } else if (currentToken.type == TOKEN_LPAREN) {
        advanceToken();
        node = parseExpression();
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')' after expression", currentToken);
        advanceToken();
    } else if (currentToken.type == TOKEN_LBRACKET) {
        node = parseArrayLiteral();
    } else if (currentToken.type == TOKEN_TRUE || currentToken.type == TOKEN_FALSE) {
        node = createAstNode(AST_BOOLEAN_LITERAL);
        parser_stats.nodes_created++;
        node->boolLiteral.value = (currentToken.type == TOKEN_TRUE);
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Created boolean literal: %s", node->boolLiteral.value ? "true" : "false");
        }
        advanceToken();
    } else {
        parserError("Unexpected token in expression", currentToken);
    }
    
    return node;
}

/**
 * Parse a curry expression: func(arg1)(arg2)...
 */
static AstNode* parseCurryExpression(AstNode* baseFunc) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseCurryExpression);
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Parsing curried function call");
    }
    
    AstNode* curryNode = createAstNode(AST_CURRY_EXPR);
    parser_stats.nodes_created++;
    
    curryNode->curryExpr.baseFunc = baseFunc;
    
    int expectedArgCount = 0;
    if (baseFunc->type == AST_FUNC_CALL) {
        expectedArgCount = 2;
    }
    
    curryNode->curryExpr.totalArgCount = expectedArgCount;
    curryNode->curryExpr.appliedArgs = NULL;
    curryNode->curryExpr.appliedCount = 0;
    
    if (baseFunc->type == AST_FUNC_CALL) {
        curryNode->curryExpr.appliedCount = baseFunc->funcCall.argCount;
    }
    
    while (currentToken.type == TOKEN_LPAREN) {
        advanceToken(); // consume '('
        
        AstNode** newArgs = NULL;
        int newArgCount = 0;
        
        while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
            AstNode *arg = parseExpression();
            newArgCount++;
            newArgs = memory_realloc(newArgs, newArgCount * sizeof(AstNode *));
            newArgs[newArgCount - 1] = arg;
            
            if (currentToken.type == TOKEN_COMMA)
                advanceToken();
            else if (currentToken.type != TOKEN_RPAREN)
                parserError("Expected ',' or ')' in curried function argument list", currentToken);
        }
        
        advanceToken(); // consume ')'
        
        int oldCount = curryNode->curryExpr.appliedCount;
        curryNode->curryExpr.appliedCount += newArgCount;
        curryNode->curryExpr.appliedArgs = memory_realloc(curryNode->curryExpr.appliedArgs,
                                                      curryNode->curryExpr.appliedCount * sizeof(AstNode *));
        
        for (int i = 0; i < newArgCount; i++) {
            curryNode->curryExpr.appliedArgs[oldCount + i] = newArgs[i];
        }
        
        free(newArgs);
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Created curry expression with %d/%d arguments applied",
                  curryNode->curryExpr.appliedCount, curryNode->curryExpr.totalArgCount);
    }
    
    return curryNode;
}

/* parseFuncDef: Parsea una definición de función */
static AstNode *parseFuncDef(void) {
    advanceToken(); // consume 'func'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected function name", currentToken);
    
    AstNode *funcNode = createAstNode(AST_FUNC_DEF);
    strncpy(funcNode->funcDef.name, currentToken.lexeme, sizeof(funcNode->funcDef.name));
    advanceToken();
    
    if (currentToken.type != TOKEN_LPAREN)
        parserError("Expected '(' after function name", currentToken);
    advanceToken();
    
    AstNode **parameters = NULL;
    int paramCount = 0;
    
    while (currentToken.type != TOKEN_RPAREN) {
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected parameter name", currentToken);
        
        AstNode *param = createAstNode(AST_IDENTIFIER);
        strncpy(param->identifier.name, currentToken.lexeme, sizeof(param->identifier.name));
        parameters = memory_realloc(parameters, (paramCount + 1) * sizeof(AstNode *));
        parameters[paramCount++] = param;
        advanceToken();
        
        if (currentToken.type == TOKEN_COLON) {
            advanceToken();
            if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
                parserError("Expected parameter type", currentToken);
            advanceToken();
        }
        
        if (currentToken.type == TOKEN_COMMA)
            advanceToken();
        else if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ',' or ')' in parameter list", currentToken);
    }
    
    advanceToken(); // consume ')'
    
    if (currentToken.type == TOKEN_ARROW) {
        advanceToken();
        if (currentToken.type != TOKEN_IDENTIFIER && currentToken.type != TOKEN_INT && currentToken.type != TOKEN_FLOAT)
            parserError("Expected return type", currentToken);
        advanceToken();
    }
    
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
        parserError("Expected 'end' to close function definition", currentToken);
    advanceToken();
    
    funcNode->funcDef.parameters = parameters;
    funcNode->funcDef.paramCount = paramCount;
    funcNode->funcDef.body = body;
    funcNode->funcDef.bodyCount = bodyCount;
    
    return funcNode;
}

/* parseClassDef: Parsea class <Name>; ... end */
static AstNode *parseClassDef(void) {
    advanceToken();  // consume 'class'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected class name", currentToken);
    
    AstNode *classNode = createAstNode(AST_CLASS_DEF);
    strncpy(classNode->classDef.name, currentToken.lexeme, sizeof(classNode->classDef.name));
    advanceToken();
    
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

/* parseLambda: ( paramName : paramType, ... ) -> returnType => bodyExpr */
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
    
    moduleNode->moduleDecl.declarations = NULL;
    moduleNode->moduleDecl.declarationCount = 0;
    
    while (currentToken.type != TOKEN_END) {
        AstNode* decl = parseStatement();
        moduleNode->moduleDecl.declarationCount++;
        moduleNode->moduleDecl.declarations = memory_realloc(moduleNode->moduleDecl.declarations,
            moduleNode->moduleDecl.declarationCount * sizeof(AstNode*));
        moduleNode->moduleDecl.declarations[moduleNode->moduleDecl.declarationCount - 1] = decl;
    }
    
    advanceToken(); // consume 'end'
    return moduleNode;
}

/* parseImport: Procesa sentencias import */
static AstNode* parseImport(void) {
    advanceToken(); // consume 'import'
    
    AstNode* importNode = createAstNode(AST_IMPORT);
    parser_stats.nodes_created++;
    
    // Inicializar campos
    importNode->importStmt.hasAlias = false;
    importNode->importStmt.hasSymbolList = false;
    importNode->importStmt.symbolCount = 0;
    importNode->importStmt.symbols = NULL;
    importNode->importStmt.aliases = NULL;
    memset(importNode->importStmt.alias, 0, sizeof(importNode->importStmt.alias));
    
    // Caso: from module import symbol1, symbol2, symbol3 as alias3...
    if (currentToken.type == TOKEN_FROM) {
        advanceToken(); // consume 'from'
        
        if (currentToken.type != TOKEN_IDENTIFIER)
            parserError("Expected module name after 'from'", currentToken);
        
        // Guardamos el nombre del módulo
        strncpy(importNode->importStmt.moduleName, currentToken.lexeme, sizeof(importNode->importStmt.moduleName) - 1);
        advanceToken(); // consume module name
        
        // Esperamos la palabra clave 'import'
        if (currentToken.type != TOKEN_IMPORT)
            parserError("Expected 'import' after module name in selective import", currentToken);
        advanceToken(); // consume 'import'
        
        // Configuramos como importación selectiva
        importNode->importStmt.hasSymbolList = true;
        
        // Procesar la lista de símbolos
        do {
            if (currentToken.type != TOKEN_IDENTIFIER)
                parserError("Expected identifier in import list", currentToken);
            
            // Incrementar contador de símbolos
            importNode->importStmt.symbolCount++;
            
            // Reasignar memoria para los arreglos de símbolos y alias
            importNode->importStmt.symbols = (const char**)memory_realloc(
                (void*)importNode->importStmt.symbols, 
                importNode->importStmt.symbolCount * sizeof(char*)
            );
            
            importNode->importStmt.aliases = (const char**)memory_realloc(
                (void*)importNode->importStmt.aliases, 
                importNode->importStmt.symbolCount * sizeof(char*)
            );
            
            if (!importNode->importStmt.symbols || !importNode->importStmt.aliases) {
                parserError("Memory allocation error in import statement", currentToken);
            }
            
            // Guardar nombre del símbolo
            importNode->importStmt.symbols[importNode->importStmt.symbolCount - 1] = memory_strdup(currentToken.lexeme);
            importNode->importStmt.aliases[importNode->importStmt.symbolCount - 1] = NULL; // Por defecto no hay alias
            
            advanceToken(); // consume nombre del símbolo
            
            // Verificar si hay un 'as' para alias
            if (currentToken.type == TOKEN_AS) {
                advanceToken(); // consume 'as'
                
                if (currentToken.type != TOKEN_IDENTIFIER)
                    parserError("Expected identifier after 'as' in import statement", currentToken);
                
                // Guardar el alias
                importNode->importStmt.aliases[importNode->importStmt.symbolCount - 1] = memory_strdup(currentToken.lexeme);
                
                advanceToken(); // consume el alias
            }
            
            // Si hay coma, hay más símbolos por importar
            if (currentToken.type == TOKEN_COMMA) {
                advanceToken(); // consume ','
            } else {
                break; // Final de la lista de símbolos
            }
        } while (1);
    }
    // Caso normal: import module o import module as alias
    else if (currentToken.type == TOKEN_IDENTIFIER) {
        // Guardamos el nombre del módulo
        strncpy(importNode->importStmt.moduleName, currentToken.lexeme, sizeof(importNode->importStmt.moduleName) - 1);
        advanceToken(); // consume module name
        
        // Comprobamos si hay un alias
        if (currentToken.type == TOKEN_AS) {
            advanceToken(); // consume 'as'
            
            if (currentToken.type != TOKEN_IDENTIFIER)
                parserError("Expected identifier after 'as' in import statement", currentToken);
            
            // Guardamos el alias y marcamos hasAlias
            strncpy(importNode->importStmt.alias, currentToken.lexeme, sizeof(importNode->importStmt.alias) - 1);
            importNode->importStmt.hasAlias = true;
            
            advanceToken(); // consume alias
        }
    } else {
        parserError("Expected module name in import statement", currentToken);
    }
    
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
        bodyCount++;
        body = realloc(body, bodyCount * sizeof(AstNode*));
        body[bodyCount - 1] = stmt;
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
        bodyCount++;
        body = realloc(body, bodyCount * sizeof(AstNode*));
        body[bodyCount - 1] = stmt;
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

/* parseSwitchStmt: switch expression case expr ... [default ...] end */
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
            if (currentToken.type == TOKEN_COLON)
                advanceToken();
            skipStatementSeparators();
            AstNode **caseBody = NULL;
            int caseBodyCount = 0;
            while (currentToken.type != TOKEN_CASE && 
                   currentToken.type != TOKEN_DEFAULT && 
                   currentToken.type != TOKEN_END && 
                   currentToken.type != TOKEN_EOF) {
                AstNode *stmt = parseStatement();
                caseBodyCount++;
                caseBody = realloc(caseBody, caseBodyCount * sizeof(AstNode*));
                caseBody[caseBodyCount - 1] = stmt;
                skipStatementSeparators();
            }
            AstNode *caseNode = createAstNode(AST_CASE_STMT);
            caseNode->caseStmt.expr = caseExpr;
            caseNode->caseStmt.body = caseBody;
            caseNode->caseStmt.bodyCount = caseBodyCount;
            caseCount++;
            cases = realloc(cases, caseCount * sizeof(AstNode*));
            cases[caseCount - 1] = caseNode;
        } else {
            advanceToken(); // consume 'default'
            if (currentToken.type == TOKEN_COLON)
                advanceToken();
            skipStatementSeparators();
            while (currentToken.type != TOKEN_CASE && 
                   currentToken.type != TOKEN_END && 
                   currentToken.type != TOKEN_EOF) {
                AstNode *stmt = parseStatement();
                defaultCaseCount++;
                defaultCase = realloc(defaultCase, defaultCaseCount * sizeof(AstNode*));
                defaultCase[defaultCaseCount - 1] = stmt;
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

/* parseTryCatchStmt: try ... catch [type] err ... [finally ...] end */
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
        tryCount++;
        tryBody = realloc(tryBody, tryCount * sizeof(AstNode*));
        tryBody[tryCount - 1] = stmt;
        skipStatementSeparators();
    }
    
    AstNode **catchBody = NULL;
    int catchCount = 0;
    char errorVarName[256] = "";
    char errorType[64] = "";  // Default empty type
    
    if (currentToken.type == TOKEN_CATCH) {
        advanceToken(); // consume 'catch'
        
        // Check for error type specification
        if (currentToken.type == TOKEN_IDENTIFIER) {
            // Store the type name
            strncpy(errorType, currentToken.lexeme, sizeof(errorType) - 1);
            advanceToken();
            
            // Check for error variable name
            if (currentToken.type == TOKEN_IDENTIFIER) {
                strncpy(errorVarName, currentToken.lexeme, sizeof(errorVarName) - 1);
                advanceToken();
            }
        } else if (currentToken.type == TOKEN_IDENTIFIER) {
            // Only error variable name provided
            strncpy(errorVarName, currentToken.lexeme, sizeof(errorVarName) - 1);
            advanceToken();
        }
        
        skipStatementSeparators();
        while (currentToken.type != TOKEN_FINALLY && 
               currentToken.type != TOKEN_END && 
               currentToken.type != TOKEN_EOF) {
            AstNode *stmt = parseStatement();
            catchCount++;
            catchBody = realloc(catchBody, catchCount * sizeof(AstNode*));
            catchBody[catchCount - 1] = stmt;
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
            finallyCount++;
            finallyBody = realloc(finallyBody, finallyCount * sizeof(AstNode*));
            finallyBody[finallyCount - 1] = stmt;
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
    strncpy(tryCatchNode->tryCatchStmt.errorType, errorType, sizeof(tryCatchNode->tryCatchStmt.errorType) - 1);
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

/* parsePatternMatch: match expr when pattern => body ... [otherwise => body] end */
static AstNode* parsePatternMatch(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parsePatternMatch);
    
    advanceToken(); // consume 'match'
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Parsing pattern match expression");
    }
    
    AstNode* matchNode = createAstNode(AST_PATTERN_MATCH);
    parser_stats.nodes_created++;
    
    matchNode->patternMatch.expr = parseExpression();
    skipStatementSeparators();
    
    matchNode->patternMatch.cases = NULL;
    matchNode->patternMatch.caseCount = 0;
    matchNode->patternMatch.otherwise = NULL;
    
    while (currentToken.type == TOKEN_WHEN) {
        advanceToken(); // consume 'when'
        
        AstNode* pattern = parseExpression();
        
        if (currentToken.type != TOKEN_FAT_ARROW) {
            parserError("Expected '=>' after pattern", currentToken);
        }
        advanceToken(); // consume '=>'
        
        AstNode** body = NULL;
        int bodyCount = 0;
        
        while (currentToken.type != TOKEN_WHEN && 
               currentToken.type != TOKEN_OTHERWISE && 
               currentToken.type != TOKEN_END && 
               currentToken.type != TOKEN_EOF) {
            AstNode* stmt = parseStatement();
            body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode*));
            body[bodyCount++] = stmt;
            skipStatementSeparators();
        }
        
        AstNode* caseNode = createAstNode(AST_PATTERN_CASE);
        parser_stats.nodes_created++;
        
        caseNode->patternCase.pattern = pattern;
        caseNode->patternCase.body = body;
        caseNode->patternCase.bodyCount = bodyCount;
        
        matchNode->patternMatch.caseCount++;
        matchNode->patternMatch.cases = memory_realloc(matchNode->patternMatch.cases, 
                                                     matchNode->patternMatch.caseCount * sizeof(AstNode*));
        matchNode->patternMatch.cases[matchNode->patternMatch.caseCount - 1] = caseNode;
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Added pattern case with %d body statements", bodyCount);
        }
    }
    
    if (currentToken.type == TOKEN_OTHERWISE) {
        advanceToken(); // consume 'otherwise'
        
        if (currentToken.type != TOKEN_FAT_ARROW) {
            parserError("Expected '=>' after 'otherwise'", currentToken);
        }
        advanceToken(); // consume '=>'
        
        AstNode** body = NULL;
        int bodyCount = 0;
        
        while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
            AstNode* stmt = parseStatement();
            body = memory_realloc(body, (bodyCount + 1) * sizeof(AstNode*));
            body[bodyCount++] = stmt;
            skipStatementSeparators();
        }
        
        AstNode* otherwiseNode = createAstNode(AST_PATTERN_CASE);
        parser_stats.nodes_created++;
        
        otherwiseNode->patternCase.pattern = NULL;
        otherwiseNode->patternCase.body = body;
        otherwiseNode->patternCase.bodyCount = bodyCount;
        
        matchNode->patternMatch.otherwise = otherwiseNode;
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Added otherwise case with %d body statements", bodyCount);
        }
    }
    
    if (currentToken.type != TOKEN_END) {
        parserError("Expected 'end' to close pattern match expression", currentToken);
    }
    advanceToken(); // consume 'end'
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Completed parsing pattern match with %d cases", 
                 matchNode->patternMatch.caseCount);
    }
    
    return matchNode;
}

/* parseAspect: Parsea una definición de aspecto */
static AstNode* parseAspect(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseAspect);
    
    advanceToken(); // consume 'aspect'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected aspect name", currentToken);
    
    AstNode* aspectNode = createAstNode(AST_ASPECT_DEF);
    strncpy(aspectNode->aspectDef.name, currentToken.lexeme, sizeof(aspectNode->aspectDef.name));
    advanceToken();
    
    skipStatementSeparators();
    
    aspectNode->aspectDef.pointcuts = NULL;
    aspectNode->aspectDef.pointcutCount = 0;
    aspectNode->aspectDef.advice = NULL;
    aspectNode->aspectDef.adviceCount = 0;
    
    while (currentToken.type != TOKEN_END) {
        if (currentToken.type == TOKEN_POINTCUT) {
            AstNode* pointcut = parsePointcut();
            aspectNode->aspectDef.pointcutCount++;
            aspectNode->aspectDef.pointcuts = memory_realloc(aspectNode->aspectDef.pointcuts,
                                                          aspectNode->aspectDef.pointcutCount * sizeof(AstNode*));
            aspectNode->aspectDef.pointcuts[aspectNode->aspectDef.pointcutCount - 1] = pointcut;
        }
        else if (currentToken.type == TOKEN_ADVICE) {
            AstNode* advice = parseAdvice();
            aspectNode->aspectDef.adviceCount++;
            aspectNode->aspectDef.advice = memory_realloc(aspectNode->aspectDef.advice,
                                                       aspectNode->aspectDef.adviceCount * sizeof(AstNode*));
            aspectNode->aspectDef.advice[aspectNode->aspectDef.adviceCount - 1] = advice;
        }
        else {
            parserError("Expected 'pointcut' or 'advice' in aspect definition", currentToken);
        }
        skipStatementSeparators();
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close aspect definition", currentToken);
    advanceToken(); // consume 'end'
    
    return aspectNode;
}

/* parsePointcut: Parsea una declaración de pointcut */
static AstNode* parsePointcut(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parsePointcut);
    
    advanceToken(); // consume 'pointcut'
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected pointcut name", currentToken);
    
    AstNode* pointcutNode = createAstNode(AST_POINTCUT);
    strncpy(pointcutNode->pointcut.name, currentToken.lexeme, sizeof(pointcutNode->pointcut.name));
    advanceToken();
    
    if (currentToken.type != TOKEN_STRING)
        parserError("Expected pattern string in pointcut definition", currentToken);
    
    strncpy(pointcutNode->pointcut.pattern, currentToken.lexeme, sizeof(pointcutNode->pointcut.pattern));
    advanceToken();
    
    skipStatementSeparators();
    return pointcutNode;
}

/* parseAdvice: Parsea una declaración de advice */
static AstNode* parseAdvice(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)parseAdvice);
    
    advanceToken(); // consume 'advice'
    
    AstNode* adviceNode = createAstNode(AST_ADVICE);
    
    if (currentToken.type == TOKEN_BEFORE)
        adviceNode->advice.type = ADVICE_BEFORE;
    else if (currentToken.type == TOKEN_AFTER)
        adviceNode->advice.type = ADVICE_AFTER;
    else if (currentToken.type == TOKEN_AROUND)
        adviceNode->advice.type = ADVICE_AROUND;
    else
        parserError("Expected advice type (before, after, or around)", currentToken);
    
    logger_log(LOG_DEBUG, "Parsing advice of type %d", adviceNode->advice.type);
    advanceToken();
    
    if (currentToken.type != TOKEN_IDENTIFIER)
        parserError("Expected pointcut name in advice declaration", currentToken);
    
    strncpy(adviceNode->advice.pointcutName, currentToken.lexeme, sizeof(adviceNode->advice.pointcutName));
    advanceToken();
    
    skipStatementSeparators();
    
    adviceNode->advice.body = NULL;
    adviceNode->advice.bodyCount = 0;
    
    while (currentToken.type != TOKEN_END) {
        AstNode* stmt = parseStatement();
        adviceNode->advice.bodyCount++;
        adviceNode->advice.body = memory_realloc(adviceNode->advice.body,
                                              adviceNode->advice.bodyCount * sizeof(AstNode*));
        adviceNode->advice.body[adviceNode->advice.bodyCount - 1] = stmt;
        skipStatementSeparators();
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close advice definition", currentToken);
    advanceToken();
    
    logger_log(LOG_DEBUG, "Completed parsing advice with %d statements", adviceNode->advice.bodyCount);
    return adviceNode;
}

static AstNode *parseReturn(void) {
    AstNode *node = createAstNode(AST_RETURN_STMT);
    if (!node) return NULL;

    nextToken(); // Skip 'return'
    
    if (currentToken.type != TOKEN_SEMICOLON) {
        node->returnStmt.expr = parseExpression();
    } else {
        node->returnStmt.expr = NULL;
    }

    expectToken(TOKEN_SEMICOLON);
    return node;
}

/* parseIfStmt: if condition ... [else ...] end */
static AstNode *parseIfStmt(void) {
    advanceToken(); // consume 'if'
    
    AstNode *condition;
    if (currentToken.type == TOKEN_LPAREN) {
        advanceToken();
        condition = parseExpression();
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')' after if condition", currentToken);
        advanceToken();
    } else {
        condition = parseExpression();
    }
    
    skipStatementSeparators();
    
    AstNode **thenBranch = NULL;
    int thenCount = 0;
    
    while (currentToken.type != TOKEN_ELSE && 
           currentToken.type != TOKEN_END && 
           currentToken.type != TOKEN_EOF) {
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
        while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
            AstNode *stmt = parseStatement();
            elseBranch = memory_realloc(elseBranch, (elseCount + 1) * sizeof(AstNode *));
            elseBranch[elseCount++] = stmt;
            skipStatementSeparators();
        }
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close if statement", currentToken);
    advanceToken();
    
    AstNode *ifNode = createAstNode(AST_IF_STMT);
    ifNode->ifStmt.condition = condition;
    ifNode->ifStmt.thenBranch = thenBranch;
    ifNode->ifStmt.thenCount = thenCount;
    ifNode->ifStmt.elseBranch = elseBranch;
    ifNode->ifStmt.elseCount = elseCount;
    
    return ifNode;
}

/* parseForStmt: for iterator in range(start, end) ... end */
static AstNode *parseForStmt(void) {
    advanceToken(); // consume 'for'
    
    AstNode *forNode = createAstNode(AST_FOR_STMT);
    
    // Determinar el tipo de bucle for
    if (currentToken.type == TOKEN_LPAREN) {
        // Caso: for (init; condition; update) - estilo C tradicional
        forNode->forStmt.forType = FOR_TRADITIONAL;
        
        advanceToken(); // consume '('
        
        // Parsear inicialización (opcional)
        if (currentToken.type != TOKEN_SEMICOLON) {
            forNode->forStmt.init = parseExpression();
        } else {
            forNode->forStmt.init = NULL;
        }
        
        if (currentToken.type != TOKEN_SEMICOLON)
            parserError("Expected ';' after initialization in for loop", currentToken);
        advanceToken(); // consume ';'
        
        // Parsear condición (opcional)
        if (currentToken.type != TOKEN_SEMICOLON) {
            forNode->forStmt.condition = parseExpression();
        } else {
            forNode->forStmt.condition = NULL;
        }
        
        if (currentToken.type != TOKEN_SEMICOLON)
            parserError("Expected ';' after condition in for loop", currentToken);
        advanceToken(); // consume ';'
        
        // Parsear actualización (opcional)
        if (currentToken.type != TOKEN_RPAREN) {
            forNode->forStmt.update = parseExpression();
        } else {
            forNode->forStmt.update = NULL;
        }
        
        if (currentToken.type != TOKEN_RPAREN)
            parserError("Expected ')' to close for loop declaration", currentToken);
        advanceToken(); // consume ')'
        
        // Inicializar otros campos para que sean NULL/0
        forNode->forStmt.iterator[0] = '\0';
        forNode->forStmt.rangeStart = NULL;
        forNode->forStmt.rangeEnd = NULL;
        forNode->forStmt.rangeStep = NULL;
        forNode->forStmt.collection = NULL;
    } 
    else if (currentToken.type == TOKEN_IDENTIFIER) {
        // Guardar el nombre del iterador
        strncpy(forNode->forStmt.iterator, currentToken.lexeme, sizeof(forNode->forStmt.iterator));
        advanceToken();
        
        if (currentToken.type != TOKEN_IN)
            parserError("Expected 'in' after iterator in for loop", currentToken);
        advanceToken(); // consume 'in'
        
        if (currentToken.type == TOKEN_RANGE) {
            // Caso: for i in range(start, end[, step])
            forNode->forStmt.forType = FOR_RANGE;
            advanceToken(); // consume 'range'
            
            if (currentToken.type != TOKEN_LPAREN)
                parserError("Expected '(' after 'range'", currentToken);
            advanceToken(); // consume '('
            
            // Parsear inicio del rango
            forNode->forStmt.rangeStart = parseExpression();
            
            if (currentToken.type == TOKEN_COMMA) {
                advanceToken(); // consume ','
                // Parsear fin del rango
                forNode->forStmt.rangeEnd = parseExpression();
                
                // Parsear paso (opcional)
                if (currentToken.type == TOKEN_COMMA) {
                    advanceToken(); // consume ','
                    forNode->forStmt.rangeStep = parseExpression();
                } else {
                    forNode->forStmt.rangeStep = NULL;
                }
            } else {
                // Solo se dio un valor: range(end)
                // Inicio implícito en 0, el valor parsado es el fin
                AstNode *zeroNode = createAstNode(AST_NUMBER_LITERAL);
                zeroNode->numberLiteral.value = 0;
                forNode->forStmt.rangeEnd = forNode->forStmt.rangeStart;
                forNode->forStmt.rangeStart = zeroNode;
                forNode->forStmt.rangeStep = NULL;
            }
            
            if (currentToken.type != TOKEN_RPAREN)
                parserError("Expected ')' after range arguments", currentToken);
            advanceToken(); // consume ')'
            
            // Inicializar otros campos que no se usan
            forNode->forStmt.collection = NULL;
            forNode->forStmt.init = NULL;
            forNode->forStmt.condition = NULL;
            forNode->forStmt.update = NULL;
        } 
        else {
            // Caso: for elem in collection
            forNode->forStmt.forType = FOR_COLLECTION;
            
            // Parsear la colección a iterar
            forNode->forStmt.collection = parseExpression();
            
            // Inicializar otros campos que no se usan
            forNode->forStmt.rangeStart = NULL;
            forNode->forStmt.rangeEnd = NULL;
            forNode->forStmt.rangeStep = NULL;
            forNode->forStmt.init = NULL;
            forNode->forStmt.condition = NULL;
            forNode->forStmt.update = NULL;
        }
    }
    else {
        parserError("Invalid for loop syntax", currentToken);
    }
    
    skipStatementSeparators();
    
    // Parsear el cuerpo del bucle for (común a todos los tipos)
    forNode->forStmt.body = NULL;
    forNode->forStmt.bodyCount = 0;
    
    while (currentToken.type != TOKEN_END && currentToken.type != TOKEN_EOF) {
        AstNode *stmt = parseStatement();
        forNode->forStmt.bodyCount++;
        forNode->forStmt.body = memory_realloc(forNode->forStmt.body, 
                                             forNode->forStmt.bodyCount * sizeof(AstNode*));
        forNode->forStmt.body[forNode->forStmt.bodyCount - 1] = stmt;
        skipStatementSeparators();
    }
    
    if (currentToken.type != TOKEN_END)
        parserError("Expected 'end' to close for loop", currentToken);
    advanceToken(); // consume 'end'
    
    return forNode;
}

// Funciones auxiliares: nextToken, expectToken, parseBlock
void nextToken(void) {
    currentToken = getNextToken();
}

void expectToken(int tokenType) {
    if (currentToken.type != tokenType) {
        char message[256];
        snprintf(message, sizeof(message), "Expected token type %d, got %d", 
                tokenType, currentToken.type);
        parserError(message, currentToken);
    }
    nextToken();
}

AstNode** parseBlock(int* count) {
    AstNode** statements = NULL;
    *count = 0;
    
    while (currentToken.type != TOKEN_END && 
           currentToken.type != TOKEN_EOF) {
        AstNode* stmt = parseStatement();
        statements = memory_realloc(statements, (*count + 1) * sizeof(AstNode*));
        statements[(*count)++] = stmt;
        skipStatementSeparators();
    }
    
    return statements;
}

void parser_set_debug_level(int level) {
    debug_level = level;
}

// Fin del parser.c
