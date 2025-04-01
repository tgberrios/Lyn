/**
 * @file memory.c
 * @brief Memory management system implementation for the Lyn compiler
 * 
 * This file implements a comprehensive memory management system that includes:
 * - Basic memory allocation wrappers with tracking
 * - Memory pooling for efficient allocation of fixed-size blocks
 * - Optional garbage collection with reference counting
 * - Memory statistics tracking and reporting
 */

#define _POSIX_C_SOURCE 200112L
#include "memory.h"
#include "error.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#ifdef USE_GC
#include <stdatomic.h>
#endif

static int debug_level = 1;  ///< Memory debug level (0=minimum, 3=maximum)

/**
 * @brief Memory statistics structure
 */
static struct {
    size_t totalAllocated;   ///< Total bytes allocated over time
    size_t currentAllocated; ///< Currently allocated bytes
    size_t allocCount;       ///< Total number of allocations
    size_t freeCount;        ///< Total number of frees
} memStats;

/**
 * @brief Sets the debug level for the memory system
 * 
 * @param level The new debug level (0-3)
 */
void memory_set_debug_level(int level) {
    debug_level = level;
    logger_log(LOG_INFO, "Memory debug level set to %d", level);
}

/**
 * @brief Gets the current debug level for the memory system
 * 
 * @return int Current debug level (0-3)
 */
int memory_get_debug_level(void) {
    return debug_level;
}

/**
 * @brief Initializes the memory management system
 * 
 * Resets all memory statistics and prepares the system for use.
 */
void memory_init(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_init);
    
    memStats.totalAllocated = 0;
    memStats.currentAllocated = 0;
    memStats.allocCount = 0;
    memStats.freeCount = 0;
    logger_log(LOG_INFO, "Memory system initialized");
}

/**
 * @brief Cleans up the memory management system
 * 
 * Checks for potential memory leaks and reports statistics.
 */
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

/**
 * @brief Prints current memory statistics
 * 
 * Displays detailed memory usage statistics including total allocations,
 * current allocations, and allocation/free counts.
 */
void memory_stats(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_stats);
    
    logger_log(LOG_INFO, "=== Memory Statistics ===");
    logger_log(LOG_INFO, "Total allocated: %zu bytes", memStats.totalAllocated);
    logger_log(LOG_INFO, "Currently allocated: %zu bytes", memStats.currentAllocated);
    logger_log(LOG_INFO, "Number of allocations: %zu", memStats.allocCount);
    logger_log(LOG_INFO, "Number of frees: %zu", memStats.freeCount);
    
    if (debug_level >= 2) {
        // Print to stderr for compatibility with existing tools
        printf("=== Memory Statistics ===\n");
        printf("Total allocated: %zu bytes\n", memStats.totalAllocated);
        printf("Currently allocated: %zu bytes\n", memStats.currentAllocated);
        printf("Number of allocations: %zu\n", memStats.allocCount);
        printf("Number of frees: %zu\n", memStats.freeCount);
        printf("============================\n");
    }
}

/* ============================
   Basic Memory Wrappers
   ============================ */

static size_t globalAllocCount = 0;  ///< Global allocation counter
static size_t globalFreeCount  = 0;  ///< Global free counter

/**
 * @brief Allocates memory with tracking
 * 
 * Wrapper for malloc that tracks allocation statistics and handles errors.
 * 
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory, or NULL if allocation failed
 */
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

/**
 * @brief Reallocates memory with tracking
 * 
 * Wrapper for realloc that handles NULL pointers and tracks statistics.
 * 
 * @param ptr Pointer to memory to reallocate
 * @param new_size New size in bytes
 * @return void* Pointer to reallocated memory, or NULL if reallocation failed
 */
void* memory_realloc(void* ptr, size_t new_size) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_realloc);
    
    // If NULL, equivalent to new allocation
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

/**
 * @brief Frees memory with tracking
 * 
 * Wrapper for free that tracks deallocation statistics.
 * 
 * @param ptr Pointer to memory to free
 */
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

/**
 * @brief Duplicates a string with tracked memory
 * 
 * @param str String to duplicate
 * @return char* New allocated string or NULL on failure
 */
char* memory_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* new_str = memory_alloc(len);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}

/* ============================
   Memory Pool Implementation
   ============================ */

/**
 * @brief Structure representing a memory pool
 */
struct MemoryPool {
    size_t blockSize;         ///< Size of each block in the pool
    size_t poolSize;          ///< Total number of blocks in the pool
    void *poolMemory;         ///< Contiguous block of allocated memory
    void *freeList;           ///< Linked list of free blocks
    size_t totalAllocs;       ///< Total number of allocations from this pool
    size_t totalFrees;        ///< Total number of frees to this pool
    pthread_mutex_t mutex;    ///< Mutex for thread safety
};

/**
 * @brief Structure for free block management
 */
typedef struct FreeBlock {
    struct FreeBlock *next;   ///< Pointer to next free block
} FreeBlock;

/**
 * @brief Creates a new memory pool
 * 
 * Allocates and initializes a memory pool with the specified block size and count.
 * 
 * @param blockSize Size of each block in the pool
 * @param poolSize Number of blocks in the pool
 * @param alignment Memory alignment requirement
 * @return MemoryPool* Pointer to the created pool, or NULL if creation failed
 */
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

/**
 * @brief Allocates a block from a memory pool
 * 
 * @param pool The memory pool to allocate from
 * @return void* Pointer to allocated block, or NULL if pool is empty
 */
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

/**
 * @brief Returns a block to a memory pool
 * 
 * @param pool The memory pool to return the block to
 * @param ptr Pointer to the block to return
 */
void memory_pool_free(MemoryPool *pool, void *ptr) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_free);
    
    if (!pool || !ptr) {
        if (!pool) logger_log(LOG_WARNING, "Attempted to free to NULL memory pool");
        return;
    }
    
    // Basic validation: confirm ptr is within pool's memory block
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

/**
 * @brief Destroys a memory pool
 * 
 * Frees all memory associated with the pool and checks for memory leaks.
 * 
 * @param pool The memory pool to destroy
 */
void memory_pool_destroy(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_destroy);
    
    if (!pool) return;
    
    // Check for memory leaks in this pool
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

/**
 * @brief Gets the total number of allocations from a pool
 * 
 * @param pool The memory pool to query
 * @return size_t Total number of allocations
 */
size_t memory_pool_get_total_allocs(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_get_total_allocs);
    return pool ? pool->totalAllocs : 0;
}

/**
 * @brief Gets the total number of frees to a pool
 * 
 * @param pool The memory pool to query
 * @return size_t Total number of frees
 */
size_t memory_pool_get_total_frees(MemoryPool *pool) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_pool_get_total_frees);
    return pool ? pool->totalFrees : 0;
}

/**
 * @brief Prints statistics for a memory pool
 * 
 * @param pool The memory pool to print statistics for
 */
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
    
    // Also print to stdout for compatibility with existing tools
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
   Global Memory Tracking
   ============================ */

/**
 * @brief Gets the global allocation count
 * 
 * @return size_t Total number of allocations across all memory operations
 */
size_t memory_get_global_alloc_count(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_get_global_alloc_count);
    return globalAllocCount;
}

/**
 * @brief Gets the global free count
 * 
 * @return size_t Total number of frees across all memory operations
 */
size_t memory_get_global_free_count(void) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)memory_get_global_free_count);
    return globalFreeCount;
}

/* ============================
   Optional Garbage Collection
   ============================ */
#ifdef USE_GC

/**
 * @brief Allocates memory with garbage collection
 * 
 * Allocates memory with reference counting for garbage collection.
 * 
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory
 */
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

/**
 * @brief Increments the reference count of a GC object
 * 
 * @param ptr Pointer to the GC object
 */
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

/**
 * @brief Decrements the reference count of a GC object
 * 
 * Frees the object if its reference count reaches zero.
 * 
 * @param ptr Pointer to the GC object
 */
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
        // If failed, 'expected' is updated; repeat the cycle.
    }
    
    char errorMsg[256];
    snprintf(errorMsg, sizeof(errorMsg), "memory_dec_ref called on ptr=%p with refCount==0", ptr);
    logger_log(LOG_ERROR, "%s", errorMsg);
    error_report("Memory", __LINE__, 0, errorMsg, ERROR_MEMORY);
    abort();
}
#endif /* USE_GC */
