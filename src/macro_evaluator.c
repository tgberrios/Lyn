#include <stdio.h>
#include <stdlib.h>
#include "macro_evaluator.h"
#include "error.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

// Añadimos estos tipos al enum AstNodeType en la implementación 
// para compatibilidad, aunque no estén en la definición oficial
#define AST_MACRO_DEF 100    // Número alto para evitar colisiones
#define AST_MACRO_PARAM 101
#define AST_MACRO_EXPAND 102

#define MAX_MACROS 1024

// Nivel de depuración
static int debug_level = 1;

// Structure to store macro definitions
typedef struct {
    char name[256];
    char** params;
    int paramCount;
    AstNode** body;
    int bodyCount;
} MacroDef;

static MacroDef macros[MAX_MACROS];
static int macroCount = 0;

void macro_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_set_debug_level);
    debug_level = level;
    logger_log(LOG_INFO, "Macro system debug level set to %d", level);
}

// Versión adaptada de register_macro que trabaja con nuestro AST actual
bool register_macro(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)register_macro);
    
    // Verificación básica
    if (!node || macroCount >= MAX_MACROS) {
        logger_log(LOG_WARNING, "Failed to register macro: %s", 
                  !node ? "NULL node" : "Too many macros");
        return false;
    }

    // Para compatibilidad con el resto del sistema, verificamos primero
    // si el nodo es una definición de función (AST_FUNC_DEF)
    // y la tratamos como una definición de macro
    if (node->type == AST_FUNC_DEF) {
        MacroDef* macro = &macros[macroCount++];
        strncpy(macro->name, node->funcDef.name, sizeof(macro->name) - 1);
        
        if (debug_level >= 1) {
            logger_log(LOG_INFO, "Registering macro: %s with %d parameters",
                     macro->name, node->funcDef.paramCount);
        }
        
        // Copy parameters
        macro->paramCount = node->funcDef.paramCount;
        macro->params = malloc(macro->paramCount * sizeof(char*));
        if (!macro->params) {
            logger_log(LOG_ERROR, "Memory allocation failed for macro parameters");
            return false;
        }
        
        for (int i = 0; i < macro->paramCount; i++) {
            if (node->funcDef.parameters[i]->type == AST_IDENTIFIER) {
                macro->params[i] = strdup(node->funcDef.parameters[i]->identifier.name);
            } else {
                macro->params[i] = strdup("unknown");
            }
        }
        
        // Copy body
        macro->bodyCount = node->funcDef.bodyCount;
        macro->body = malloc(macro->bodyCount * sizeof(AstNode*));
        if (!macro->body) {
            logger_log(LOG_ERROR, "Memory allocation failed for macro body");
            // Liberar memoria de los parámetros
            for (int i = 0; i < macro->paramCount; i++) {
                free(macro->params[i]);
            }
            free(macro->params);
            return false;
        }
        
        for (int i = 0; i < macro->bodyCount; i++) {
            macro->body[i] = node->funcDef.body[i];
        }
        
        return true;
    }
    
    // Si no es una función, podría ser un nodo AST_MACRO_DEF específico
    // (esto es para mantener compatibilidad con código que use AST_MACRO_DEF)
    if (node->type == AST_MACRO_DEF) {
        logger_log(LOG_WARNING, "AST_MACRO_DEF is deprecated, use AST_FUNC_DEF instead");
        // Aquí iría el código original pero adaptado a nuestra estructura
        return false;  // Por ahora no soportamos esta opción
    }
    
    logger_log(LOG_WARNING, "Node type %d is not supported for macro registration", node->type);
    return false;
}

static MacroDef* find_macro(const char* name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)find_macro);
    
    for (int i = 0; i < macroCount; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return &macros[i];
        }
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Macro not found: %s", name);
    }
    
    return NULL;
}

AstNode* expand_macro(const char* name, AstNode** args, int argCount) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)expand_macro);
    
    MacroDef* macro = find_macro(name);
    if (!macro) {
        logger_log(LOG_WARNING, "Attempted to expand undefined macro: %s", name);
        return NULL;
    }
    
    if (macro->paramCount != argCount) {
        logger_log(LOG_WARNING, "Macro %s expects %d arguments, but %d were provided",
                  name, macro->paramCount, argCount);
        return NULL;
    }
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "Expanding macro: %s with %d arguments", name, argCount);
    }
    
    // Create new scope for macro expansion
    // Map parameters to arguments
    AstNode** expanded = malloc(macro->bodyCount * sizeof(AstNode*));
    if (!expanded) {
        logger_log(LOG_ERROR, "Memory allocation failed for macro expansion");
        return NULL;
    }
    
    // Simple parameter substitution strategy
    for (int i = 0; i < macro->bodyCount; i++) {
        // Create a deep copy of each statement to avoid modifying the original
        expanded[i] = copyAstNode(macro->body[i]);
        
        // For real substitution, we would need to traverse the AST and replace
        // all occurrences of parameter names with the corresponding arguments
        // This is a simplified version
    }
    
    // Create block node with expanded macro body
    AstNode* block = createAstNode(AST_PROGRAM);
    if (!block) {
        logger_log(LOG_ERROR, "Failed to create block node for macro expansion");
        for (int i = 0; i < macro->bodyCount; i++) {
            freeAstNode(expanded[i]);
        }
        free(expanded);
        return NULL;
    }
    
    block->program.statements = expanded;
    block->program.statementCount = macro->bodyCount;
    
    if (debug_level >= 1) {
        logger_log(LOG_INFO, "Macro %s expanded to %d statements", name, macro->bodyCount);
    }
    
    return block;
}

char* macro_stringify(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_stringify);
    
    if (!node) {
        logger_log(LOG_WARNING, "Attempted to stringify NULL node");
        return strdup("NULL");
    }
    
    char buffer[1024];
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            snprintf(buffer, sizeof(buffer), "%g", node->numberLiteral.value);
            break;
        case AST_STRING_LITERAL:
            snprintf(buffer, sizeof(buffer), "\"%s\"", node->stringLiteral.value);
            break;
        case AST_IDENTIFIER:
            snprintf(buffer, sizeof(buffer), "%s", node->identifier.name);
            break;
        case AST_BOOLEAN_LITERAL:
            snprintf(buffer, sizeof(buffer), "%s", node->boolLiteral.value ? "true" : "false");
            break;
        default:
            return strdup("<<unprintable>>");
    }
    
    return strdup(buffer);
}

char* macro_concat(const char* s1, const char* s2) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_concat);
    
    if (!s1 || !s2) {
        logger_log(LOG_WARNING, "Attempted to concat with NULL string");
        return strdup(s1 ? s1 : (s2 ? s2 : ""));
    }
    
    size_t len = strlen(s1) + strlen(s2) + 1;
    char* result = malloc(len);
    if (!result) {
        logger_log(LOG_ERROR, "Memory allocation failed for string concatenation");
        return NULL;
    }
    
    snprintf(result, len, "%s%s", s1, s2);
    return result;
}

// Versión adaptada para trabajar con nuestro AST actual
AstNode* evaluate_macros(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)evaluate_macros);
    
    if (!node) return NULL;

    // En nuestro AST actual usamos AST_FUNC_DEF en lugar de AST_MACRO_DEF
    // Podemos reconocer macros por convención, por ejemplo, por un prefijo en el nombre
    if (node->type == AST_FUNC_DEF) {
        // Si el nombre empieza con "macro_", lo consideramos una macro
        if (strncmp(node->funcDef.name, "macro_", 6) == 0) {
            if (register_macro(node)) {
                // Retornar NULL para eliminar la definición del AST final
                // ya que ha sido registrada como macro
                return NULL;
            }
        }
    }
    
    // Para compatibilidad con el código original
    if (node->type == AST_MACRO_DEF) {
        if (register_macro(node)) {
            return NULL; // Remove macro definition from AST
        }
    }
    
    // Para compatibilidad con el código original
    if (node->type == AST_MACRO_EXPAND) {
        // Este sería el caso original, pero para nuestro AST actual
        // usaríamos una llamada a función con nombre especial
        return NULL;
    }
    
    // Detectamos llamadas a macros por el nombre de la función
    if (node->type == AST_FUNC_CALL) {
        MacroDef* macro = find_macro(node->funcCall.name);
        if (macro) {
            // Es una llamada a una macro registrada
            return expand_macro(node->funcCall.name, 
                               node->funcCall.arguments, 
                               node->funcCall.argCount);
        }
    }
    
    // Recursively process children
    switch (node->type) {
        case AST_PROGRAM:
            // Procesar cada sentencia y reemplazar si es necesario
            {
                int newCount = 0;
                AstNode** newStatements = malloc(node->program.statementCount * sizeof(AstNode*));
                if (!newStatements) {
                    logger_log(LOG_ERROR, "Memory allocation failed for macro processing");
                    return node;
                }
                
                for (int i = 0; i < node->program.statementCount; i++) {
                    AstNode* result = evaluate_macros(node->program.statements[i]);
                    if (result) {
                        // Si el resultado es un programa (expansión de macro),
                        // incorporar sus sentencias directamente
                        if (result->type == AST_PROGRAM) {
                            for (int j = 0; j < result->program.statementCount; j++) {
                                newStatements[newCount++] = result->program.statements[j];
                            }
                            // Liberar el nodo programa pero no sus sentencias
                            free(result->program.statements);
                            free(result);
                        } else {
                            newStatements[newCount++] = result;
                        }
                    }
                }
                
                // Liberar el array original pero no sus elementos
                free(node->program.statements);
                node->program.statements = newStatements;
                node->program.statementCount = newCount;
            }
            break;
            
        case AST_FUNC_DEF:
            // Procesar el cuerpo de la función
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                AstNode* result = evaluate_macros(node->funcDef.body[i]);
                if (result) {
                    node->funcDef.body[i] = result;
                }
            }
            break;
            
        case AST_IF_STMT:
            // Procesar condición
            {
                AstNode* result = evaluate_macros(node->ifStmt.condition);
                if (result) {
                    node->ifStmt.condition = result;
                }
            }
            
            // Procesar rama 'then'
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                AstNode* result = evaluate_macros(node->ifStmt.thenBranch[i]);
                if (result) {
                    node->ifStmt.thenBranch[i] = result;
                }
            }
            
            // Procesar rama 'else'
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                AstNode* result = evaluate_macros(node->ifStmt.elseBranch[i]);
                if (result) {
                    node->ifStmt.elseBranch[i] = result;
                }
            }
            break;
            
        // Añadir otros tipos de nodos según sea necesario
            
        default:
            // Para otros tipos de nodos que no contienen hijos, no hacemos nada
            break;
    }
    
    return node;
}

// Inicialización del sistema de macros
void macro_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_init);
    
    // Reiniciar contador de macros
    macroCount = 0;
    
    logger_log(LOG_INFO, "Macro system initialized");
}

// Limpieza del sistema de macros
void macro_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)macro_cleanup);
    
    // Liberar memoria de todas las macros
    for (int i = 0; i < macroCount; i++) {
        for (int j = 0; j < macros[i].paramCount; j++) {
            free(macros[i].params[j]);
        }
        free(macros[i].params);
        
        // No liberamos los cuerpos de las macros porque son referencias
        // a nodos que pertenecen al AST principal
        free(macros[i].body);
    }
    
    macroCount = 0;
    
    logger_log(LOG_INFO, "Macro system cleanup complete");
}
