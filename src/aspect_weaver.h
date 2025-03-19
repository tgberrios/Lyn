#ifndef ASPECT_WEAVER_H
#define ASPECT_WEAVER_H

#include "ast.h"
#include <stdbool.h>

// Estructura para almacenar información del weaving
typedef struct {
    int joinpoints_found;     // Número de puntos de unión encontrados
    int advice_applied;       // Número de advice aplicados
    char error_msg[256];     // Mensaje de error si algo falla
} WeavingStats;

// Inicializa el weaver
void weaver_init(void);

// Establece el nivel de depuración
void weaver_set_debug_level(int level);

// Realiza el proceso de weaving sobre el AST
bool weaver_process(AstNode* ast);

// Obtiene estadísticas del último proceso de weaving
WeavingStats weaver_get_stats(void);

// Limpia recursos del weaver
void weaver_cleanup(void);

#endif /* ASPECT_WEAVER_H */
