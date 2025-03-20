#include "aspect_weaver.h"
#include "logger.h"
#include "error.h"
#include "memory.h"
#include <string.h>
#include <stdlib.h>

// Usamos la enumeración AdviceType definida en ast.h en lugar de defines
#include "ast.h"

// Definiciones de los tipos de advice
#define ADVICE_BEFORE 0
#define ADVICE_AFTER  1
#define ADVICE_AROUND 2

// Nivel de depuración del weaver
static int debug_level = 0;

// Estadísticas del proceso de weaving
static WeavingStats stats = {0};

// Lista de aspectos encontrados durante el análisis
typedef struct {
    AstNode** aspects;
    int count;
} AspectList;

static AspectList aspect_list = {NULL, 0};

// Prototipos de funciones internas
static bool collect_aspects(AstNode* ast);
static bool apply_aspects(AstNode* ast);
static bool matches_pointcut(const char* pattern, const char* target);
static AstNode* clone_advice_body(AstNode* advice);
static void insert_advice(AstNode* target, AstNode* advice, int position);

void weaver_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_init);
    
    // Reiniciar estadísticas
    stats = (WeavingStats){0};
    
    // Limpiar lista de aspectos
    if (aspect_list.aspects) {
        free(aspect_list.aspects);
        aspect_list.aspects = NULL;
    }
    aspect_list.count = 0;
    
    logger_log(LOG_INFO, "Aspect weaver initialized");
}

void weaver_set_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_set_debug_level);
    
    debug_level = level;
    logger_log(LOG_INFO, "Aspect weaver debug level set to %d", level);
}

bool weaver_process(AstNode* ast) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_process);
    
    if (!ast) {
        strncpy(stats.error_msg, "NULL AST provided", sizeof(stats.error_msg)-1);
        return false;
    }
    
    logger_log(LOG_INFO, "Starting aspect weaving process");
    
    // Paso 1: Recolectar todos los aspectos del programa
    if (!collect_aspects(ast)) {
        return false;
    }
    
    if (aspect_list.count == 0) {
        logger_log(LOG_INFO, "No aspects found in the program");
        return true;  // No hay aspectos, pero no es un error
    }
    
    logger_log(LOG_INFO, "Found %d aspects in the program", aspect_list.count);
    
    // Paso 2: Aplicar los aspectos encontrados
    if (!apply_aspects(ast)) {
        return false;
    }
    
    logger_log(LOG_INFO, "Aspect weaving completed: found %d joinpoints, applied %d advice",
              stats.joinpoints_found, stats.advice_applied);
    
    return true;
}

WeavingStats weaver_get_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_get_stats);
    return stats;
}

void weaver_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)weaver_cleanup);
    
    // Liberar memoria de la lista de aspectos
    if (aspect_list.aspects) {
        // No liberamos los nodos AST aquí ya que son parte del AST principal
        free(aspect_list.aspects);
        aspect_list.aspects = NULL;
    }
    aspect_list.count = 0;
    
    logger_log(LOG_INFO, "Aspect weaver cleanup completed");
}

static bool collect_aspects(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)collect_aspects);
    
    if (!node) return true;
    
    // Si encontramos un aspecto, lo agregamos a la lista
    if (node->type == AST_ASPECT_DEF) {
        aspect_list.count++;
        aspect_list.aspects = memory_realloc(aspect_list.aspects, 
                                           aspect_list.count * sizeof(AstNode*));
        if (!aspect_list.aspects) {
            strncpy(stats.error_msg, "Memory allocation failed", sizeof(stats.error_msg)-1);
            return false;
        }
        aspect_list.aspects[aspect_list.count - 1] = node;
        
        if (debug_level >= 2) {
            logger_log(LOG_DEBUG, "Collected aspect: %s with %d pointcuts and %d advice", 
                      node->aspectDef.name, node->aspectDef.pointcutCount, node->aspectDef.adviceCount);
        }
    }
    
    // Recursivamente buscar en los hijos
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!collect_aspects(node->program.statements[i])) return false;
            }
            break;
            
        case AST_FUNC_DEF:
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (!collect_aspects(node->funcDef.body[i])) return false;
            }
            break;
            
        case AST_IF_STMT:
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                if (!collect_aspects(node->ifStmt.thenBranch[i])) return false;
            }
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                if (!collect_aspects(node->ifStmt.elseBranch[i])) return false;
            }
            break;
            
        case AST_WHILE_STMT:
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                if (!collect_aspects(node->whileStmt.body[i])) return false;
            }
            break;
            
        case AST_DO_WHILE_STMT:
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                if (!collect_aspects(node->doWhileStmt.body[i])) return false;
            }
            break;
            
        case AST_FOR_STMT:
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                if (!collect_aspects(node->forStmt.body[i])) return false;
            }
            break;
            
        // Añadir otros casos según sea necesario
    }
    
    return true;
}

static bool apply_aspects(AstNode* node) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)apply_aspects);
    
    if (!node) return true;
    
    // Si es una definición de función, verificar si coincide con algún pointcut
    if (node->type == AST_FUNC_DEF) {
        logger_log(LOG_DEBUG, "Checking function '%s' for aspect application", node->funcDef.name);
        
        for (int i = 0; i < aspect_list.count; i++) {
            AstNode* aspect = aspect_list.aspects[i];
            
            for (int j = 0; j < aspect->aspectDef.pointcutCount; j++) {
                AstNode* pointcut = aspect->aspectDef.pointcuts[j];
                
                // Mejorar el logging para ver los patrones que se están evaluando
                logger_log(LOG_DEBUG, "Checking if '%s' matches pattern '%s'", 
                          node->funcDef.name, pointcut->pointcut.pattern);
                
                if (matches_pointcut(pointcut->pointcut.pattern, node->funcDef.name)) {
                    stats.joinpoints_found++;
                    
                    logger_log(LOG_INFO, "Found joinpoint: %s matches %s",
                             node->funcDef.name, pointcut->pointcut.pattern);
                    
                    // Aplicar todos los advice asociados a este pointcut
                    for (int k = 0; k < aspect->aspectDef.adviceCount; k++) {
                        AstNode* advice = aspect->aspectDef.advice[k];
                        
                        if (strcmp(advice->advice.pointcutName, pointcut->pointcut.name) == 0) {
                            // Clonar el cuerpo del advice
                            AstNode* advice_body = clone_advice_body(advice);
                            if (!advice_body) {
                                strncpy(stats.error_msg, "Failed to clone advice body", sizeof(stats.error_msg)-1);
                                return false;
                            }
                            
                            // Insertar el advice según su tipo
                            switch (advice->advice.type) {
                                case ADVICE_BEFORE:
                                    logger_log(LOG_INFO, "Applying BEFORE advice to %s", node->funcDef.name);
                                    insert_advice(node, advice_body, 0);
                                    break;
                                    
                                case ADVICE_AFTER:
                                    logger_log(LOG_INFO, "Applying AFTER advice to %s", node->funcDef.name);
                                    insert_advice(node, advice_body, -1);
                                    break;
                                    
                                case ADVICE_AROUND:
                                    logger_log(LOG_INFO, "Applying AROUND advice to %s (treating as before)", node->funcDef.name);
                                    insert_advice(node, advice_body, 0);
                                    break;
                                    
                                default:
                                    logger_log(LOG_WARNING, "Unknown advice type: %d", advice->advice.type);
                                    freeAstNode(advice_body);
                                    continue;
                            }
                            
                            stats.advice_applied++;
                            
                            const char* adviceTypeStr;
                            switch (advice->advice.type) {
                                case ADVICE_BEFORE: adviceTypeStr = "before"; break;
                                case ADVICE_AFTER:  adviceTypeStr = "after"; break;
                                case ADVICE_AROUND: adviceTypeStr = "around"; break;
                                default: adviceTypeStr = "unknown"; break;
                            }
                            logger_log(LOG_INFO, "Applied %s advice to %s",
                                      adviceTypeStr,
                                      node->funcDef.name);
                        }
                    }
                }
            }
        }
    }
    
    // Recursivamente procesar los hijos
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.statementCount; i++) {
                if (!apply_aspects(node->program.statements[i])) return false;
            }
            break;
            
        case AST_FUNC_DEF:
            // Procesar el cuerpo de la función después de aplicar aspectos
            for (int i = 0; i < node->funcDef.bodyCount; i++) {
                if (!apply_aspects(node->funcDef.body[i])) return false;
            }
            break;
            
        case AST_IF_STMT:
            for (int i = 0; i < node->ifStmt.thenCount; i++) {
                if (!apply_aspects(node->ifStmt.thenBranch[i])) return false;
            }
            for (int i = 0; i < node->ifStmt.elseCount; i++) {
                if (!apply_aspects(node->ifStmt.elseBranch[i])) return false;
            }
            break;
            
        case AST_WHILE_STMT:
            for (int i = 0; i < node->whileStmt.bodyCount; i++) {
                if (!apply_aspects(node->whileStmt.body[i])) return false;
            }
            break;
            
        case AST_DO_WHILE_STMT:
            for (int i = 0; i < node->doWhileStmt.bodyCount; i++) {
                if (!apply_aspects(node->doWhileStmt.body[i])) return false;
            }
            break;
            
        case AST_FOR_STMT:
            for (int i = 0; i < node->forStmt.bodyCount; i++) {
                if (!apply_aspects(node->forStmt.body[i])) return false;
            }
            break;
            
        // Añadir otros casos según sea necesario
    }
    
    return true;
}

static bool matches_pointcut(const char* pattern, const char* target) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)matches_pointcut);
    
    logger_log(LOG_DEBUG, "Matching '%s' against pattern '%s'", target, pattern);
    
    // Caso especial para patrones terminados en "*" como "test_*"
    size_t pattern_len = strlen(pattern);
    size_t target_len = strlen(target);
    
    if (pattern_len > 0 && pattern[pattern_len - 1] == '*') {
        // Comparar los caracteres antes del '*'
        size_t prefix_len = pattern_len - 1;
        if (target_len >= prefix_len && strncmp(target, pattern, prefix_len) == 0) {
            logger_log(LOG_INFO, "Match found! Function '%s' matches pattern '%s'", 
                      target, pattern);
            return true;
        }
    }
    
    // Coincidencia exacta
    if (strcmp(pattern, target) == 0) {
        logger_log(LOG_INFO, "Exact match found! Function '%s' matches pattern '%s'", 
                  target, pattern);
        return true;
    }
    
    // El resto del código original para coincidencias más complejas...
    const char* p = pattern;
    const char* t = target;
    
    // Algoritmo más complejo para manejar comodines en medio del patrón
    while (*p && *t) {
        if (*p == '*') {
            p++;
            if (!*p) return true; // * al final coincide con todo
            
            // Buscar la siguiente parte después del *
            while (*t) {
                if (matches_pointcut(p, t)) return true;
                t++;
            }
            return false;
        }
        else if (*p != *t) {
            return false;
        }
        p++;
        t++;
    }
    
    return *p == *t; // Ambos deben haber terminado
}

static AstNode* clone_advice_body(AstNode* advice) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)clone_advice_body);
    
    if (!advice || advice->type != AST_ADVICE) {
        return NULL;
    }
    
    // Crear un nodo de bloque para contener todas las sentencias del advice
    AstNode* block = createAstNode(AST_BLOCK);
    if (!block) {
        return NULL;
    }
    
    // Copiar cada sentencia del advice al bloque
    block->block.statements = (AstNode**)memory_alloc(sizeof(AstNode*) * advice->advice.bodyCount);
    block->block.statementCount = advice->advice.bodyCount;
    
    for (int i = 0; i < advice->advice.bodyCount; i++) {
        // Aquí estamos utilizando copyAstNode para hacer una copia real
        // de cada sentencia en el cuerpo del advice
        block->block.statements[i] = copyAstNode(advice->advice.body[i]);
        if (!block->block.statements[i]) {
            // Si falla la copia, liberar lo que ya se ha copiado
            for (int j = 0; j < i; j++) {
                freeAstNode(block->block.statements[j]);
            }
            memory_free(block->block.statements);
            freeAstNode(block);
            return NULL;
        }
    }
    
    return block;
}

static void insert_advice(AstNode* target, AstNode* advice, int position) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)insert_advice);
    
    if (target->type != AST_FUNC_DEF || !advice) return;
    
    // Calcular la posición de inserción
    int insert_pos = (position >= 0) ? position : target->funcDef.bodyCount;
    
    // Hacer espacio para el nuevo advice
    target->funcDef.bodyCount++;
    target->funcDef.body = memory_realloc(target->funcDef.body,
                                         target->funcDef.bodyCount * sizeof(AstNode*));
    
    if (!target->funcDef.body) {
        logger_log(LOG_ERROR, "Memory allocation failed when inserting advice");
        return;
    }
    
    // Desplazar los elementos existentes si es necesario
    if (insert_pos < target->funcDef.bodyCount - 1) {
        memmove(&target->funcDef.body[insert_pos + 1],
                &target->funcDef.body[insert_pos],
                (target->funcDef.bodyCount - insert_pos - 1) * sizeof(AstNode*));
    }
    
    // Insertar el advice
    target->funcDef.body[insert_pos] = advice;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Inserted advice at position %d in function %s",
                  insert_pos, target->funcDef.name);
    }
}
