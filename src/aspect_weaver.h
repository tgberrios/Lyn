#ifndef ASPECT_WEAVER_H
#define ASPECT_WEAVER_H

#include "ast.h"
#include <stdbool.h>

// Estadísticas del proceso de weaving
typedef struct {
    int joinpoints_found;  // Número de puntos de unión encontrados
    int advice_applied;    // Número total de advice aplicados
    char error_msg[256];   // Mensaje de error si ocurre alguno
} WeavingStats;

// Inicializa el tejedor de aspectos
void weaver_init(void);

// Establece el nivel de depuración
void weaver_set_debug_level(int level);

// Procesa un AST para aplicar aspectos
bool weaver_process(AstNode* ast);

// Obtiene estadísticas del proceso de weaving
WeavingStats weaver_get_stats(void);

// Limpia los recursos utilizados por el tejedor
void weaver_cleanup(void);

#endif /* ASPECT_WEAVER_H */
