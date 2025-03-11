#define _POSIX_C_SOURCE 200112L
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifdef USE_GC
#include <stdatomic.h>
#endif

#include "logger.h"

// Estadísticas de memoria
static struct {
    size_t totalAllocated;
    size_t currentAllocated;
    size_t allocCount;
    size_t freeCount;
} memStats;

void memory_init(void) {
    memStats.totalAllocated = 0;
    memStats.currentAllocated = 0;
    memStats.allocCount = 0;
    memStats.freeCount = 0;
    logger_log(LOG_INFO, "Sistema de memoria inicializado");
}

void memory_cleanup(void) {
    if (memStats.allocCount != memStats.freeCount) {
        logger_log(LOG_WARNING, "Posible fuga de memoria: %zu asignaciones, %zu liberaciones",
                  memStats.allocCount, memStats.freeCount);
    }
    logger_log(LOG_INFO, "Sistema de memoria liberado");
}

void memory_stats(void) {
    printf("=== Estadísticas de Memoria ===\n");
    printf("Total asignado: %zu bytes\n", memStats.totalAllocated);
    printf("Actualmente asignado: %zu bytes\n", memStats.currentAllocated);
    printf("Número de asignaciones: %zu\n", memStats.allocCount);
    printf("Número de liberaciones: %zu\n", memStats.freeCount);
    printf("============================\n");
}

/* ============================
   Wrappers Básicos de Memoria
   ============================ */

static size_t globalAllocCount = 0;
static size_t globalFreeCount  = 0;

void* memory_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        logger_log(LOG_ERROR, "Fallo en asignación de memoria de %zu bytes", size);
        return NULL;
    }
    
    memStats.totalAllocated += size;
    memStats.currentAllocated += size;
    memStats.allocCount++;
    
    logger_log(LOG_DEBUG, "Asignados %zu bytes en %p", size, ptr);
    return ptr;
}

void* memory_realloc(void* ptr, size_t new_size) {
    // Si es NULL, equivale a una asignación nueva
    if (!ptr) {
        return memory_alloc(new_size);
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        logger_log(LOG_ERROR, "Fallo en reasignación de memoria de %zu bytes", new_size);
        return NULL;
    }
    
    logger_log(LOG_DEBUG, "Reasignados %zu bytes de %p a %p", new_size, ptr, new_ptr);
    return new_ptr;
}

void memory_free(void* ptr) {
    if (!ptr) return;
    
    free(ptr);
    memStats.freeCount++;
    
    logger_log(LOG_DEBUG, "Memoria liberada en %p", ptr);
}

/* ============================
   Implementación del Memory Pooling
   ============================ */

struct MemoryPool {
    size_t blockSize;         /* Tamaño de cada bloque */
    size_t poolSize;          /* Número total de bloques */
    void *poolMemory;         /* Bloque contiguo de memoria asignado */
    void *freeList;           /* Lista enlazada de bloques libres */
    size_t totalAllocs;       /* Número total de asignaciones realizadas */
    size_t totalFrees;        /* Número total de liberaciones realizadas */
    pthread_mutex_t mutex;    /* Mutex para thread-safety */
};

typedef struct FreeBlock {
    struct FreeBlock *next;
} FreeBlock;

MemoryPool *memory_pool_create(size_t blockSize, size_t poolSize, size_t alignment) {
    if (blockSize < sizeof(FreeBlock *))
        blockSize = sizeof(FreeBlock *);

    MemoryPool *pool = (MemoryPool *)memory_alloc(sizeof(MemoryPool));
    pool->blockSize = blockSize;
    pool->poolSize = poolSize;
    pool->totalAllocs = 0;
    pool->totalFrees = 0;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize mutex in memory pool.\n");
        memory_free(pool);
        return NULL;
    }

    int res = posix_memalign(&pool->poolMemory, alignment, blockSize * poolSize);
    if (res != 0) {
        fprintf(stderr, "Error: posix_memalign failed with error %d\n", res);
        pthread_mutex_destroy(&pool->mutex);
        memory_free(pool);
        return NULL;
    }
    memset(pool->poolMemory, 0, blockSize * poolSize);

    pool->freeList = NULL;
    for (size_t i = 0; i < poolSize; i++) {
        void *block = (char *)pool->poolMemory + i * blockSize;
        ((FreeBlock *)block)->next = pool->freeList;
        pool->freeList = block;
    }
#ifdef DEBUG_MEMORY
    fprintf(stderr, "[memory_pool_create] pool=%p blockSize=%zu poolSize=%zu alignment=%zu\n",
            pool, blockSize, poolSize, alignment);
#endif
    return pool;
}

void *memory_pool_alloc(MemoryPool *pool) {
    void *block = NULL;
    pthread_mutex_lock(&pool->mutex);
    if (pool->freeList) {
        block = pool->freeList;
        pool->freeList = ((FreeBlock *)block)->next;
        pool->totalAllocs++;
#ifdef DEBUG_MEMORY
        fprintf(stderr, "[memory_pool_alloc] pool=%p block=%p (totalAllocs=%zu)\n",
                pool, block, pool->totalAllocs);
#endif
    }
    pthread_mutex_unlock(&pool->mutex);
    return block;
}

void memory_pool_free(MemoryPool *pool, void *ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&pool->mutex);
    ((FreeBlock *)ptr)->next = pool->freeList;
    pool->freeList = ptr;
    pool->totalFrees++;
#ifdef DEBUG_MEMORY
    fprintf(stderr, "[memory_pool_free] pool=%p block=%p (totalFrees=%zu)\n",
            pool, ptr, pool->totalFrees);
#endif
    pthread_mutex_unlock(&pool->mutex);
}

void memory_pool_destroy(MemoryPool *pool) {
    if (!pool) return;
    pthread_mutex_destroy(&pool->mutex);
    memory_free(pool->poolMemory);
    memory_free(pool);
}

size_t memory_pool_get_total_allocs(MemoryPool *pool) {
    return pool ? pool->totalAllocs : 0;
}

size_t memory_pool_get_total_frees(MemoryPool *pool) {
    return pool ? pool->totalFrees : 0;
}

void memory_pool_dumpStats(MemoryPool *pool) {
    if (!pool) return;
    printf("Memory Pool Stats:\n");
    printf("  Block size   : %zu\n", pool->blockSize);
    printf("  Pool size    : %zu\n", pool->poolSize);
    printf("  Allocs       : %zu\n", pool->totalAllocs);
    printf("  Frees        : %zu\n", pool->totalFrees);
    size_t inUse = (pool->totalAllocs > pool->totalFrees)
                    ? (pool->totalAllocs - pool->totalFrees)
                    : 0;
    printf("  Blocks in use: %zu\n", inUse);
    printf("  Pool pointer : %p\n", (void*)pool);
}

/* ============================
   Tracking Global de Memoria
   ============================ */

size_t memory_get_global_alloc_count(void) {
    return globalAllocCount;
}

size_t memory_get_global_free_count(void) {
    return globalFreeCount;
}

/* ============================
   Garbage Collection Opcional
   ============================ */
#ifdef USE_GC

/**
 * @brief Asigna memoria gestionada por el GC.
 *
 * Reserva un bloque que incluye un encabezado GCHeader seguido de la data solicitada.
 *
 * @param size Tamaño de la data.
 * @return void* Puntero a la data (después del encabezado).
 */
void* memory_alloc_gc(size_t size) {
    size_t totalSize = sizeof(GCHeader) + size;
    GCHeader *header = (GCHeader *)malloc(totalSize);
    if (!header) {
        fprintf(stderr, "Error: memory_alloc_gc failed to allocate %zu bytes\n", totalSize);
        exit(EXIT_FAILURE);
    }
    atomic_init(&header->refCount, 1);
#ifdef DEBUG_MEMORY
    globalAllocCount++;
    fprintf(stderr, "[memory_alloc_gc] header=%p data_ptr=%p size=%zu, refcount=%zu (globalAllocCount=%zu)\n",
            header, (void*)(header + 1), size, atomic_load(&header->refCount), globalAllocCount);
#endif
    return (void*)(header + 1);
}

/**
 * @brief Incrementa el contador de referencias de un objeto GC.
 */
void memory_inc_ref(void *ptr) {
    if (ptr) {
        GCHeader *header = ((GCHeader *)ptr) - 1;
        size_t oldCount = atomic_fetch_add(&header->refCount, 1);
#ifdef DEBUG_MEMORY
        fprintf(stderr, "[memory_inc_ref] ptr=%p old refCount=%zu, new refCount=%zu\n",
                ptr, oldCount, atomic_load(&header->refCount));
#endif
    }
}

/**
 * @brief Decrementa el contador de referencias y libera el objeto si llega a cero.
 *
 * Se usa un ciclo de compare_exchange para evitar problemas de concurrencia y se aborta
 * si se detecta que el contador ya es 0 (lo que indicaría un doble free).
 */
void memory_dec_ref(void *ptr) {
    if (!ptr) return;
    GCHeader *header = ((GCHeader *)ptr) - 1;
    size_t expected = atomic_load(&header->refCount);
    while (expected > 0) {
        if (atomic_compare_exchange_weak(&header->refCount, &expected, expected - 1)) {
#ifdef DEBUG_MEMORY
            fprintf(stderr, "[memory_dec_ref] ptr=%p old refCount=%zu\n", ptr, expected);
#endif
            if (expected == 1) {
                free(header);
#ifdef DEBUG_MEMORY
                globalFreeCount++;
                fprintf(stderr, "[memory_dec_ref] ptr=%p freed (globalFreeCount=%zu)\n", ptr, globalFreeCount);
#endif
            }
            return;
        }
        // Si falla, 'expected' se actualiza; repetir el ciclo.
    }
    fprintf(stderr, "Error: memory_dec_ref called on ptr=%p with refCount==0\n", ptr);
    abort();
}
#endif /* USE_GC */
