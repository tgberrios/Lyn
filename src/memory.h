/**
 * @file memory.h
 * @brief Memory management system interface for the Lyn compiler
 * 
 * This header file defines the interface for a comprehensive memory management system
 * that includes basic memory allocation wrappers, memory pooling for fixed-size objects,
 * optional garbage collection, and memory usage tracking.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include "error.h"   // Error handling system
#include "logger.h"  // Logging system

#ifdef __cplusplus
extern "C" {
#endif

/* ============================
   Basic Memory Management
   ============================ */

/**
 * @brief Initializes the memory management system
 * 
 * Resets all memory statistics and prepares the system for use.
 */
void memory_init(void);

/**
 * @brief Cleans up the memory management system
 * 
 * Checks for potential memory leaks and reports statistics.
 */
void memory_cleanup(void);

/**
 * @brief Prints current memory statistics
 * 
 * Displays detailed memory usage statistics including total allocations,
 * current allocations, and allocation/free counts.
 */
void memory_stats(void);

/**
 * @brief Sets the debug level for the memory system
 * 
 * @param level The new debug level (0-3)
 */
void memory_set_debug_level(int level);

/**
 * @brief Gets the current debug level for the memory system
 * 
 * @return int Current debug level (0-3)
 */
int memory_get_debug_level(void);

/**
 * @brief Allocates memory with tracking
 * 
 * Wrapper for malloc that tracks allocation statistics and handles errors.
 * 
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory, or NULL if allocation failed
 */
void* memory_alloc(size_t size);

/**
 * @brief Reallocates memory with tracking
 * 
 * Wrapper for realloc that handles NULL pointers and tracks statistics.
 * 
 * @param ptr Pointer to memory to reallocate
 * @param new_size New size in bytes
 * @return void* Pointer to reallocated memory, or NULL if reallocation failed
 */
void* memory_realloc(void* ptr, size_t new_size);

/**
 * @brief Frees memory with tracking
 * 
 * Wrapper for free that tracks deallocation statistics.
 * 
 * @param ptr Pointer to memory to free
 */
void memory_free(void* ptr);

/**
 * @brief Duplicates a string with tracked memory
 * 
 * @param str String to duplicate
 * @return char* New allocated string or NULL on failure
 */
char* memory_strdup(const char* str);

/* ============================
   Memory Pooling for Fixed-Size Objects
   ============================ */

/**
 * @brief Structure representing a memory pool for fixed-size objects
 * 
 * The pool reserves a contiguous block of memory, divides it into fixed-size blocks,
 * maintains a list of free blocks for reuse, and tracks statistics.
 */
typedef struct MemoryPool MemoryPool;

/**
 * @brief Creates a memory pool for fixed-size objects
 * 
 * Allocates and initializes a memory pool with the specified block size and count.
 * 
 * @param blockSize Size of each block in bytes
 * @param poolSize Total number of blocks to reserve
 * @param alignment Required alignment (e.g., 16, 32, or 64 bytes)
 * @return MemoryPool* Pointer to the created pool, or NULL if creation failed
 */
MemoryPool *memory_pool_create(size_t blockSize, size_t poolSize, size_t alignment);

/**
 * @brief Allocates a block from the memory pool
 * 
 * Returns a free block or NULL if no blocks are available.
 * 
 * @param pool Pointer to the memory pool
 * @return void* Pointer to the allocated block
 */
void *memory_pool_alloc(MemoryPool *pool);

/**
 * @brief Returns a block to the memory pool
 * 
 * @param pool Pointer to the memory pool
 * @param ptr Pointer to the block to return
 */
void memory_pool_free(MemoryPool *pool, void *ptr);

/**
 * @brief Destroys the memory pool and frees all resources
 * 
 * @param pool Pointer to the memory pool to destroy
 */
void memory_pool_destroy(MemoryPool *pool);

/**
 * @brief Gets the total number of allocations from the pool
 * 
 * @param pool Pointer to the memory pool
 * @return size_t Number of allocations
 */
size_t memory_pool_get_total_allocs(MemoryPool *pool);

/**
 * @brief Gets the total number of frees to the pool
 * 
 * @param pool Pointer to the memory pool
 * @return size_t Number of frees
 */
size_t memory_pool_get_total_frees(MemoryPool *pool);

/**
 * @brief Prints statistics for the memory pool
 * 
 * Displays information about block size, number of blocks, allocations, and frees.
 * 
 * @param pool Pointer to the memory pool
 */
void memory_pool_dumpStats(MemoryPool *pool);

/* ============================
   Global Memory Tracking
   ============================ */

/**
 * @brief Gets the total number of global allocations
 * 
 * @return size_t Number of global allocations
 */
size_t memory_get_global_alloc_count(void);

/**
 * @brief Gets the total number of global frees
 * 
 * @return size_t Number of global frees
 */
size_t memory_get_global_free_count(void);

/* ============================
   Optional Garbage Collection (USE_GC)
   ============================ */
#ifdef USE_GC
#include <stdatomic.h>

/**
 * @brief Header structure for GC-managed objects
 * 
 * Stored immediately before the allocated data.
 */
typedef struct GCHeader {
    atomic_size_t refCount;  ///< Reference count for garbage collection
} GCHeader;

/**
 * @brief Allocates memory managed by the garbage collector
 * 
 * Reserves a block with space for the header and data.
 * 
 * @param size Size of the requested data
 * @return void* Pointer to the data (after the header)
 */
void* memory_alloc_gc(size_t size);

/**
 * @brief Increments the reference count of a GC object
 * 
 * @param ptr Pointer to the object's data
 */
void memory_inc_ref(void *ptr);

/**
 * @brief Decrements the reference count and frees the object if it reaches zero
 * 
 * @param ptr Pointer to the object's data
 */
void memory_dec_ref(void *ptr);
#endif /* USE_GC */

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */
