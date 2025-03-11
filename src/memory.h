#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memory_alloc(size_t size);
void* memory_realloc(void* ptr, size_t new_size);
void memory_free(void* ptr);

/* ============================
   Memory Pooling para Objetos Fijos
   ============================ */

/**
 * @brief Estructura que representa un pool de memoria para objetos de tamaño fijo.
 *
 * El pool reserva un bloque contiguo de memoria, lo divide en bloques fijos,
 * mantiene una lista de bloques libres para reutilización y registra estadísticas.
 */
typedef struct MemoryPool MemoryPool;

/**
 * @brief Crea un pool de memoria para objetos de tamaño fijo.
 *
 * @param blockSize Tamaño de cada bloque en bytes.
 * @param poolSize Número total de bloques a reservar.
 * @param alignment Alineación requerida (por ejemplo, 16, 32 o 64 bytes).
 * @return MemoryPool* Puntero al pool creado o NULL en caso de error.
 */
MemoryPool *memory_pool_create(size_t blockSize, size_t poolSize, size_t alignment);

/**
 * @brief Asigna un bloque de memoria desde el pool.
 *
 * Retorna un bloque libre o NULL si no hay bloques disponibles.
 *
 * @param pool Puntero al pool.
 * @return void* Puntero al bloque asignado.
 */
void *memory_pool_alloc(MemoryPool *pool);

/**
 * @brief Libera un bloque, devolviéndolo al pool.
 *
 * @param pool Puntero al pool.
 * @param ptr Puntero al bloque a liberar.
 */
void memory_pool_free(MemoryPool *pool, void *ptr);

/**
 * @brief Destruye el pool de memoria y libera todos sus recursos.
 *
 * @param pool Puntero al pool a destruir.
 */
void memory_pool_destroy(MemoryPool *pool);

/**
 * @brief Obtiene el número total de asignaciones realizadas desde el pool.
 *
 * @param pool Puntero al pool.
 * @return size_t Número de asignaciones.
 */
size_t memory_pool_get_total_allocs(MemoryPool *pool);

/**
 * @brief Obtiene el número total de liberaciones realizadas en el pool.
 *
 * @param pool Puntero al pool.
 * @return size_t Número de liberaciones.
 */
size_t memory_pool_get_total_frees(MemoryPool *pool);

/**
 * @brief Imprime estadísticas del pool de memoria.
 *
 * Muestra información sobre el tamaño de bloque, número de bloques, asignaciones y liberaciones.
 *
 * @param pool Puntero al pool.
 */
void memory_pool_dumpStats(MemoryPool *pool);

/* ============================
   Tracking Global de Memoria
   ============================ */

/**
 * @brief Retorna el número total de asignaciones globales realizadas.
 *
 * @return size_t Número de asignaciones globales.
 */
size_t memory_get_global_alloc_count(void);

/**
 * @brief Retorna el número total de liberaciones globales realizadas.
 *
 * @return size_t Número de liberaciones globales.
 */
size_t memory_get_global_free_count(void);

/* ============================
   Garbage Collection Opcional (USE_GC)
   ============================ */
#ifdef USE_GC
#include <stdatomic.h>

/**
 * Estructura de encabezado para objetos gestionados por GC.
 * Se almacena justo antes de los datos asignados.
 */
typedef struct GCHeader {
    atomic_size_t refCount;
} GCHeader;

/**
 * @brief Asigna memoria gestionada por el GC.
 *
 * Reserva un bloque con espacio para el encabezado y la data.
 *
 * @param size Tamaño de la data solicitada.
 * @return void* Puntero a la data (después del header).
 */
void* memory_alloc_gc(size_t size);

/**
 * @brief Incrementa el contador de referencias del objeto.
 *
 * @param ptr Puntero a la data del objeto.
 */
void memory_inc_ref(void *ptr);

/**
 * @brief Decrementa el contador de referencias y libera el objeto si llega a cero.
 *
 * @param ptr Puntero a la data del objeto.
 */
void memory_dec_ref(void *ptr);
#endif /* USE_GC */

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */
