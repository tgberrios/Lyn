#define _POSIX_C_SOURCE 200112L
#include "memory.h"
#include "error.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifdef USE_GC
#include <stdatomic.h>
#endif

// Nivel de depuración para memoria (0=mínimo, 3=máximo)
static int debug_level = 1;

// Estadísticas de memoria
static struct {
    size_t totalAllocated;
    size_t currentAllocated;
    size_t allocCount;
    size_t freeCount;
} memStats;

void memory_set_debug_level(int level) {
    debug_level = level;
    logger_log(LOG_INFO, "Memory debug level set to %d", level);
}

int memory_get_debug_level(void) {
    return debug_level;
}

void memory_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_init);
    
    memStats.totalAllocated = 0;
    memStats.currentAllocated = 0;
    memStats.allocCount = 0;
    memStats.freeCount = 0;
    logger_log(LOG_INFO, "Memory system initialized");
}

void memory_cleanup(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_cleanup);
    
    if (memStats.allocCount != memStats.freeCount) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
                "Possible memory leak: %zu allocations, %zu frees, difference: %zu",
                memStats.allocCount, memStats.freeCount, memStats.allocCount - memStats.freeCount);
        
        logger_log(LOG_WARNING, "%s", msg);
        error_report("Memory", __LINE__, 0, msg, ERROR_MEMORY);
    }
    logger_log(LOG_INFO, "Memory system cleanup complete");
}

void memory_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_stats);
    
    logger_log(LOG_INFO, "=== Memory Statistics ===");
    logger_log(LOG_INFO, "Total allocated: %zu bytes", memStats.totalAllocated);
    logger_log(LOG_INFO, "Currently allocated: %zu bytes", memStats.currentAllocated);
    logger_log(LOG_INFO, "Number of allocations: %zu", memStats.allocCount);
    logger_log(LOG_INFO, "Number of frees: %zu", memStats.freeCount);
    
    if (debug_level >= 2) {
        // Imprimir a stderr para compatibilidad con herramientas existentes
        printf("=== Memory Statistics ===\n");
        printf("Total allocated: %zu bytes\n", memStats.totalAllocated);
        printf("Currently allocated: %zu bytes\n", memStats.currentAllocated);
        printf("Number of allocations: %zu\n", memStats.allocCount);
        printf("Number of frees: %zu\n", memStats.freeCount);
        printf("============================\n");
    }
}

/* ============================
   Wrappers Básicos de Memoria
   ============================ */

static size_t globalAllocCount = 0;
static size_t globalFreeCount  = 0;

void* memory_alloc(size_t size) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_alloc);
    
    void* ptr = malloc(size);
    if (!ptr) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to allocate %zu bytes", size);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
        return NULL;
    }
    
    memStats.totalAllocated += size;
    memStats.currentAllocated += size;
    memStats.allocCount++;
    globalAllocCount++;
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Allocated %zu bytes at %p", size, ptr);
    }
    return ptr;
}

void* memory_realloc(void* ptr, size_t new_size) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_realloc);
    
    // Si es NULL, equivale a una asignación nueva
    if (!ptr) {
        return memory_alloc(new_size);
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to reallocate %zu bytes", new_size);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
        return NULL;
    }
    
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "Reallocated from %p to %p (%zu bytes)", ptr, new_ptr, new_size);
    }
    return new_ptr;
}

void memory_free(void* ptr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_free);
    
    if (!ptr) return;
    
    free(ptr);
    memStats.freeCount++;
    globalFreeCount++;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Freed memory at %p", ptr);
    }
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
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_create);
    
    if (blockSize < sizeof(FreeBlock *))
        blockSize = sizeof(FreeBlock *);

    MemoryPool *pool = (MemoryPool *)memory_alloc(sizeof(MemoryPool));
    if (!pool) {
        error_report("Memory", __LINE__, 0, "Failed to allocate memory for pool structure", ERROR_MEMORY);
        return NULL;
    }
    
    pool->blockSize = blockSize;
    pool->poolSize = poolSize;
    pool->totalAllocs = 0;
    pool->totalFrees = 0;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to initialize mutex in memory pool");
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
        memory_free(pool);
        return NULL;
    }

    int res = posix_memalign(&pool->poolMemory, alignment, blockSize * poolSize);
    if (res != 0) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "posix_memalign failed with error %d", res);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
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
    
    logger_log(LOG_INFO, "Created memory pool: %p (block size: %zu, count: %zu)", 
             pool, blockSize, poolSize);
    return pool;
}

void *memory_pool_alloc(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_alloc);
    
    if (!pool) {
        logger_log(LOG_ERROR, "Attempted to allocate from NULL memory pool");
        error_report("Memory", __LINE__, 0, "Null pool in memory_pool_alloc", ERROR_MEMORY);
        return NULL;
    }
    
    void *block = NULL;
    pthread_mutex_lock(&pool->mutex);
    if (pool->freeList) {
        block = pool->freeList;
        pool->freeList = ((FreeBlock *)block)->next;
        pool->totalAllocs++;
        
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "Pool %p allocated block %p (total: %zu)", 
                     pool, block, pool->totalAllocs);
        }
    } else {
        logger_log(LOG_WARNING, "Memory pool %p is out of blocks", pool);
    }
    pthread_mutex_unlock(&pool->mutex);
    return block;
}

void memory_pool_free(MemoryPool *pool, void *ptr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_free);
    
    if (!pool || !ptr) {
        if (!pool) logger_log(LOG_WARNING, "Attempted to free to NULL memory pool");
        return;
    }
    
    // Validación básica: confirmar que ptr está dentro del bloque de memoria del pool
    if (ptr < pool->poolMemory || 
        ptr >= (void*)((char*)pool->poolMemory + pool->blockSize * pool->poolSize)) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Invalid pointer %p returned to pool %p (outside range)", 
                ptr, pool);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
        return;
    }
    
    pthread_mutex_lock(&pool->mutex);
    ((FreeBlock *)ptr)->next = pool->freeList;
    pool->freeList = ptr;
    pool->totalFrees++;
    
    if (debug_level >= 3) {
        logger_log(LOG_DEBUG, "Pool %p freed block %p (total: %zu)", 
                 pool, ptr, pool->totalFrees);
    }
    pthread_mutex_unlock(&pool->mutex);
}

void memory_pool_destroy(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_destroy);
    
    if (!pool) return;
    
    // Verificar si hay fugas de memoria en este pool
    if (pool->totalAllocs > pool->totalFrees) {
        char warnMsg[256];
        snprintf(warnMsg, sizeof(warnMsg), 
                "Memory pool %p destroyed with %zu unreleased blocks", 
                pool, pool->totalAllocs - pool->totalFrees);
        logger_log(LOG_WARNING, "%s", warnMsg);
    }
    
    pthread_mutex_destroy(&pool->mutex);
    memory_free(pool->poolMemory);
    memory_free(pool);
    
    logger_log(LOG_INFO, "Memory pool %p destroyed", pool);
}

size_t memory_pool_get_total_allocs(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_get_total_allocs);
    return pool ? pool->totalAllocs : 0;
}

size_t memory_pool_get_total_frees(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_get_total_frees);
    return pool ? pool->totalFrees : 0;
}

void memory_pool_dumpStats(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_dumpStats);
    
    if (!pool) {
        logger_log(LOG_WARNING, "Attempted to dump stats of NULL memory pool");
        return;
    }
    
    size_t inUse = (pool->totalAllocs > pool->totalFrees)
                    ? (pool->totalAllocs - pool->totalFrees)
                    : 0;
                    
    logger_log(LOG_INFO, "Memory Pool %p Stats:", pool);
    logger_log(LOG_INFO, "  Block size   : %zu", pool->blockSize);
    logger_log(LOG_INFO, "  Pool size    : %zu", pool->poolSize);
    logger_log(LOG_INFO, "  Allocs       : %zu", pool->totalAllocs);
    logger_log(LOG_INFO, "  Frees        : %zu", pool->totalFrees);
    logger_log(LOG_INFO, "  Blocks in use: %zu", inUse);
    
    // Imprimir también a stdout para compatibilidad con herramientas existentes
    if (debug_level >= 1) {
        printf("Memory Pool Stats:\n");
        printf("  Block size   : %zu\n", pool->blockSize);
        printf("  Pool size    : %zu\n", pool->poolSize);
        printf("  Allocs       : %zu\n", pool->totalAllocs);
        printf("  Frees        : %zu\n", pool->totalFrees);
        printf("  Blocks in use: %zu\n", inUse);
        printf("  Pool pointer : %p\n", (void*)pool);
    }
}

/* ============================
   Tracking Global de Memoria
   ============================ */

size_t memory_get_global_alloc_count(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_get_global_alloc_count);
    return globalAllocCount;
}

size_t memory_get_global_free_count(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_get_global_free_count);
    return globalFreeCount;
}

/* ============================
   Garbage Collection Opcional
   ============================ */
#ifdef USE_GC

void* memory_alloc_gc(size_t size) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_alloc_gc);
    
    size_t totalSize = sizeof(GCHeader) + size;
    GCHeader *header = (GCHeader *)malloc(totalSize);
    if (!header) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to allocate %zu bytes for GC object", totalSize);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
        exit(EXIT_FAILURE);
    }
    atomic_init(&header->refCount, 1);
    
    globalAllocCount++;
    if (debug_level >= 2) {
        logger_log(LOG_DEBUG, "GC allocated %zu bytes at %p (data: %p)", 
                 size, (void*)header, (void*)(header + 1));
    }
    return (void*)(header + 1);
}

void memory_inc_ref(void *ptr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_inc_ref);
    
    if (ptr) {
        GCHeader *header = ((GCHeader *)ptr) - 1;
        size_t oldCount = atomic_fetch_add(&header->refCount, 1);
        if (debug_level >= 3) {
            logger_log(LOG_DEBUG, "GC increased refcount for %p: %zu -> %zu", 
                     ptr, oldCount, atomic_load(&header->refCount));
        }
    }
}

void memory_dec_ref(void *ptr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_dec_ref);
    
    if (!ptr) return;
    GCHeader *header = ((GCHeader *)ptr) - 1;
    size_t expected = atomic_load(&header->refCount);
    while (expected > 0) {
        if (atomic_compare_exchange_weak(&header->refCount, &expected, expected - 1)) {
            if (debug_level >= 3) {
                logger_log(LOG_DEBUG, "GC decreased refcount for %p: %zu -> %zu", 
                         ptr, expected, expected - 1);
            }
            
            if (expected == 1) {
                free(header);
                globalFreeCount++;
                if (debug_level >= 2) {
                    logger_log(LOG_DEBUG, "GC freed object %p (header: %p)", ptr, (void*)header);
                }
            }
            return;
        }
        // Si falla, 'expected' se actualiza; repetir el ciclo.
    }
    
    char errorMsg[256];
    snprintf(errorMsg, sizeof(errorMsg), "memory_dec_ref called on ptr=%p with refCount==0", ptr);
    logger_log(LOG_ERROR, "%s", errorMsg);
    error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
    abort();
}
#endif /* USE_GC */
