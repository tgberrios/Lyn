/**
 * @file main.c
 * @brief Main entry point for the Lyn compiler
 * 
 * This file contains the main function and supporting functions for the Lyn compiler.
 * It handles command-line argument parsing, file operations, and orchestrates the
 * compilation process including lexing, parsing, optimization, and code generation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "optimizer.h"
#include "logger.h"
#include "error.h"
#include "memory.h"  // For managed memory functions
#include "types.h"   // For type system integration
#include "aspect_weaver.h"  // Include aspect weaver header
#include <unistd.h>
#include <getopt.h>  // Include explicitly for optarg and optind

// Declare these variables externally for IntelliSense recognition
extern char *optarg;
extern int optind, opterr, optopt;

static int debug_level = 1;  ///< Global debug level (0=minimum, 3=maximum)

/**
 * @brief Sets the debug level across all compiler modules
 * 
 * Updates the debug level in all compiler components to ensure consistent
 * logging behavior throughout the compilation process.
 * 
 * @param level The new debug level to set (0-3)
 */
static void set_global_debug_level(int level) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)set_global_debug_level);
    
    debug_level = level;
    lexer_set_debug_level(level);
    parser_set_debug_level(level);
    compiler_set_debug_level(level);
    optimizer_set_debug_level(level);
    types_set_debug_level(level);
    
    logger_log(LOG_INFO, "Global debug level set to %d", level);
}

/**
 * @brief Reads a source file with integrated error handling
 * 
 * Opens and reads the contents of a source file, handling various error
 * conditions that might occur during file operations.
 * 
 * @param path Path to the source file to read
 * @return char* Contents of the file as a string, or NULL if an error occurred
 */
char* readFile(const char* path) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)readFile);
    
    logger_log(LOG_INFO, "Reading source file: %s", path);
    
    FILE* file = fopen(path, "r");
    if (!file) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Could not open file: %s", path);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("FileIO", __LINE__, 0, errorMsg, ERROR_IO);
        error_print_current();
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "File is empty or invalid: %s", path);
        logger_log(LOG_ERROR, "%s", errorMsg);
        error_report("FileIO", __LINE__, 0, errorMsg, ERROR_IO);
        error_print_current();
        fclose(file);
        return NULL;
    }

    char* buffer = malloc(size + 1);
    if (!buffer) {
        logger_log(LOG_ERROR, "Memory allocation failed for file buffer (%ld bytes)", size + 1);
        error_report("Memory", __LINE__, 0, "Memory allocation failed for file buffer", ERROR_MEMORY);
        error_print_current();
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    buffer[bytesRead] = '\0';
    
    if (bytesRead < (size_t)size) {
        logger_log(LOG_WARNING, "Read fewer bytes than expected: %zu/%ld", bytesRead, size);
    }
    
    fclose(file);
    logger_log(LOG_DEBUG, "File read successfully: %zu bytes", bytesRead);
    return buffer;
}

/**
 * @brief Compiles generated C code into an executable
 * 
 * Uses gcc to compile the generated C code into an executable file.
 * 
 * @param outputPath Path to the generated C source file
 * @param executablePath Path where the executable should be created
 * @return int 0 if compilation was successful, non-zero otherwise
 */
int compileOutputC(const char* outputPath, const char* executablePath) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)compileOutputC);
    
    logger_log(LOG_INFO, "Compiling C code '%s' to executable '%s'", outputPath, executablePath);
    
    char command[1024];
    // Add -lm explicitly to ensure math library linking
    snprintf(command, sizeof(command), "gcc -o %s %s -lm -Wall", executablePath, outputPath);
    
    logger_log(LOG_DEBUG, "Executing compiler command: %s", command);
    
    int result = system(command);
    if (result != 0) {
        logger_log(LOG_ERROR, "C compilation failed with status %d", result);
    } else {
        logger_log(LOG_INFO, "C compilation successful");
    }
    
    return result;
}

/**
 * @brief Prints usage information for the compiler
 * 
 * Displays command-line options and usage instructions to the user.
 * 
 * @param program_name Name of the compiler executable
 */
void print_usage(const char* program_name) {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)print_usage);
    
    fprintf(stderr, "Usage: %s [options] <source_file>\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d <level>  Set debug level (0-3, default 1)\n");
    fprintf(stderr, "  -o <level>  Set optimization level (0-2, default 1)\n");
    fprintf(stderr, "  -h          Show this help message\n");
    fprintf(stderr, "  -v          Show version information\n");
    
    logger_log(LOG_INFO, "Help information displayed");
}

/**
 * @brief Prints version information for the compiler
 * 
 * Displays the compiler's version number and copyright information.
 */
void print_version() {
    error_push_debug(__func__, __FILE__, __LINE__, (void*)print_version);
    
    fprintf(stderr, "Lyn Compiler version 0.1.0\n");
    fprintf(stderr, "Copyright (c) 2023\n");
    
    logger_log(LOG_INFO, "Version information displayed");
}

/**
 * @brief Main entry point for the Lyn compiler
 * 
 * Orchestrates the entire compilation process, including:
 * - Command-line argument parsing
 * - Source file reading
 * - Lexical analysis
 * - Parsing
 * - Type checking
 * - Optimization
 * - Code generation
 * - Aspect weaving
 * - C code compilation
 * - Program execution
 * 
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return int 0 if compilation was successful, non-zero otherwise
 */
int main(int argc, char* argv[]) {
    // Initialize logger
    logger_init("lyn_compiler.log");
    logger_set_level(LOG_DEBUG);
    
    logger_log(LOG_INFO, "Lyn compiler starting");
    error_push_debug(__func__, __FILE__, __LINE__, (void*)main);
    
    // Parse command-line options
    int debug_opt = 1;         // Default debug level
    int optimization_level = 1; // Default optimization level
    int opt;
    
    while ((opt = getopt(argc, argv, "d:o:hv")) != -1) {
        switch (opt) {
            case 'd':
                debug_opt = atoi(optarg);
                if (debug_opt < 0 || debug_opt > 3) {
                    logger_log(LOG_ERROR, "Invalid debug level: %d. Must be between 0 and 3", debug_opt);
                    return 1;
                }
                break;
                
            case 'o':
                optimization_level = atoi(optarg);
                if (optimization_level < 0 || optimization_level > 2) {
                    logger_log(LOG_ERROR, "Invalid optimization level: %d. Must be between 0 and 2", optimization_level);
                    return 1;
                }
                break;
                
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            case 'v':
                print_version();
                return 0;
                
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Set the global debug level for all modules
    set_global_debug_level(debug_opt);
    
    // Check if we have a source file
    if (optind >= argc) {
        logger_log(LOG_ERROR, "No source file specified");
        error_report("Main", __LINE__, 0, "No source file provided", ERROR_IO);
        error_print_current();
        print_usage(argv[0]);
        return 1;
    }

    char* sourcePath = argv[optind];
    logger_log(LOG_INFO, "Starting compilation of %s", sourcePath);
    logger_log(LOG_INFO, "Debug level: %d, Optimization level: %d", debug_opt, optimization_level);

    // Get base filename for output file paths
    char* baseName = strdup(sourcePath);
    if (!baseName) {
        logger_log(LOG_ERROR, "Memory allocation failed for basename");
        error_report("Memory", __LINE__, 0, "Memory allocation failed for basename", ERROR_MEMORY);
        error_print_current();
        return 1;
    }
    
    // Strip extension if present
    char* dot = strrchr(baseName, '.');
    if (dot) *dot = '\0';

    // Create output file paths
    char outputPath[256];
    char executablePath[256];
    snprintf(outputPath, sizeof(outputPath), "%s.c", baseName);
    snprintf(executablePath, sizeof(executablePath), "%s.out", baseName);
    
    logger_log(LOG_DEBUG, "Output C file: %s", outputPath);
    logger_log(LOG_DEBUG, "Output executable: %s", executablePath);

    // Read source file with improved error handling
    char* source = readFile(sourcePath);
    if (!source) {
        // Error already reported by readFile
        free(baseName);
        return 1;
    }
    
    // Set source in error system for context extraction
    error_set_source(source);

    // Initialize core components
    logger_log(LOG_INFO, "Initializing compiler components");
    
    // Initialize the lexer before using it
    lexerInitialize();
    
    lexerInit(source);
    optimizer_init((OptimizerLevel)optimization_level);

    // Parse source code
    logger_log(LOG_INFO, "Parsing source code...");
    AstNode* ast = parseProgram();
    if (!ast) {
        logger_log(LOG_ERROR, "Parsing failed");
        error_report(sourcePath, 0, 0, "Parsing failed - invalid syntax", ERROR_SYNTAX);
        error_print_current();
        free(source);
        free(baseName);
        return 1;
    }

    logger_log(LOG_DEBUG, "Source code read: %d bytes", (int)strlen(source));
    logger_log(LOG_INFO, "Source parsed successfully");

    // Initialize and run the aspect weaver
    logger_log(LOG_INFO, "Initializing aspect weaver...");
    weaver_init();
    weaver_set_debug_level(debug_opt); // Use command-line debug level

    // Process AST with aspect weaver
    if (!weaver_process(ast)) {
        WeavingStats weaving_stats = weaver_get_stats();
        logger_log(LOG_WARNING, "Aspect weaving encountered issues: %s", weaving_stats.error_msg);
        logger_log(LOG_WARNING, "Continuing with compilation anyway");
    } else {
        WeavingStats weaving_stats = weaver_get_stats();
        logger_log(LOG_INFO, "Aspect weaving complete: %d join points found, %d advice applied",
                 weaving_stats.joinpoints_found, weaving_stats.advice_applied);
    }

    // Perform type checking
    logger_log(LOG_INFO, "Performing type checking...");
    Type* programType = infer_type(ast);
    if (programType && programType->kind != TYPE_UNKNOWN) {
        logger_log(LOG_INFO, "Type checking successful");
    } else {
        logger_log(LOG_WARNING, "Type checking resulted in unknown type, proceeding with caution");
    }

    // Optimize AST
    logger_log(LOG_INFO, "Optimizing AST...");
    AstNode* optimized_ast = optimize_ast(ast);
    
    // Print optimization statistics if debug level is high enough
    if (debug_level >= 2) {
        OptimizationStats stats = optimizer_get_stats();
        logger_log(LOG_DEBUG, "Optimizations applied: %d", stats.total_optimizations);
        logger_log(LOG_DEBUG, "   Constants folded: %d", stats.constant_folding_applied);
        logger_log(LOG_DEBUG, "   Dead code removed: %d", stats.dead_code_removed);
        logger_log(LOG_DEBUG, "   Redundant assignments: %d", stats.redundant_assignments_removed);
    }

    // Generate C code
    logger_log(LOG_INFO, "Generating C code: %s", outputPath);
    if (!compileToC(optimized_ast, outputPath)) {
        logger_log(LOG_ERROR, "C code generation failed");
        error_report(sourcePath, 0, 0, "Failed to generate C code", ERROR_RUNTIME);
        error_print_current();
        freeAst(optimized_ast);
        free(source);
        free(baseName);
        return 1;
    }
    
    // Report compiler statistics
    CompilerStats comp_stats = compiler_get_stats();
    logger_log(LOG_INFO, "Compiler statistics: %d nodes processed, %d functions compiled",
              comp_stats.nodes_processed, comp_stats.functions_compiled);

    // Compile generated C code to executable
    logger_log(LOG_INFO, "Compiling C code to executable...");
    printf("Compiling %s to %s...\n", outputPath, executablePath);
    if (compileOutputC(outputPath, executablePath) != 0) {
        logger_log(LOG_ERROR, "C compilation failed");
        error_report(sourcePath, 0, 0, "C compilation failed", ERROR_RUNTIME);
        error_print_current();
        freeAst(optimized_ast);
        free(source);
        free(baseName);
        return 1;
    }

    // Run compiled program
    logger_log(LOG_INFO, "Running compiled program...");
    printf("Running %s:\n", executablePath);
    printf("----------------------------------------\n");
    char runCommand[1024];
    snprintf(runCommand, sizeof(runCommand), "./%s", executablePath);
    int result = system(runCommand);
    printf("----------------------------------------\n");
    
    if (result != 0) {
        logger_log(LOG_ERROR, "Program execution failed with return code: %d", result);
        error_report(sourcePath, 0, 0, "Program execution failed", ERROR_RUNTIME);
        error_print_current();
    } else {
        logger_log(LOG_INFO, "Program executed successfully");
    }

    // Clean up
    logger_log(LOG_DEBUG, "Cleaning up resources...");
    if (optimized_ast) {
        freeAst(optimized_ast);
    }
    
    free(source);
    free(baseName);

    // Clean up aspect weaver
    weaver_cleanup();

    logger_log(LOG_INFO, "Compilation completed successfully");
    logger_close();
    
    printf("Compilation and execution completed!\n");
    
    return (result == 0) ? 0 : 1;
}
