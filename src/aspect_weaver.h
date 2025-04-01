#ifndef ASPECT_WEAVER_H
#define ASPECT_WEAVER_H

#include "ast.h"
#include <stdbool.h>

/**
 * @brief Statistics of the weaving process
 */
typedef struct {
    int joinpoints_found;  ///< Number of joinpoints found
    int advice_applied;    ///< Total number of advice applied
    char error_msg[256];   ///< Error message if any occurs
} WeavingStats;

/**
 * @brief Initializes the aspect weaver
 * 
 * This function prepares the weaver for a new weaving process by:
 * 1. Resetting all statistics
 * 2. Cleaning up any existing aspect list
 * 3. Setting up the logging system
 */
void weaver_init(void);

/**
 * @brief Sets the debug level for the weaver
 * 
 * @param level Debug level (0-3)
 *             0: No logs
 *             1: Basic logs
 *             2: Detailed logs
 *             3: Very detailed logs
 */
void weaver_set_debug_level(int level);

/**
 * @brief Processes an AST to apply aspects
 * 
 * This is the main entry point for the aspect weaving process. It:
 * 1. Collects all aspects defined in the program
 * 2. Identifies joinpoints that match the pointcuts
 * 3. Applies the corresponding advice
 * 
 * @param ast Root AST node of the program
 * @return true if the process was successful, false otherwise
 */
bool weaver_process(AstNode* ast);

/**
 * @brief Gets the current weaving process statistics
 * 
 * @return WeavingStats structure containing:
 *         - Number of joinpoints found
 *         - Number of advice applied
 *         - Any error messages
 */
WeavingStats weaver_get_stats(void);

/**
 * @brief Cleans up resources used by the weaver
 * 
 * This function:
 * 1. Frees memory allocated for the aspect list
 * 2. Resets all internal state
 * 3. Prepares the system for a new weaving process
 */
void weaver_cleanup(void);

#endif /* ASPECT_WEAVER_H */
