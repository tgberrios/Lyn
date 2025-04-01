/**
 * @file parser.h
 * @brief Parser interface for the Lyn programming language
 * 
 * This header file defines the interface for the parser component of the Lyn compiler.
 * The parser is responsible for converting source code into an Abstract Syntax Tree (AST).
 * 
 * Features:
 * - Parsing of basic expressions and statements
 * - Function and class definitions
 * - Control structures (if, for, while, etc.)
 * - Pattern matching
 * - Aspect-oriented programming features
 * - Module system
 * - Lambda expressions
 * - Object-oriented features
 * 
 * The parser uses a recursive descent approach and maintains internal state
 * for tracking tokens and parsing progress.
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "error.h"
#include "logger.h"

/**
 * @brief Parses the source code and returns the root of the AST
 * 
 * This is the main entry point for parsing a Lyn program. It handles:
 * - Top-level function definitions
 * - Main program block
 * - Module declarations
 * - Import statements
 * 
 * The function expects the program to start with a 'main' block and
 * handles the parsing of all statements within it.
 * 
 * @return AstNode* Pointer to the root node of the AST
 * @throws parserError if syntax errors are encountered
 */
AstNode *parseProgram(void);

/**
 * @brief Sets the debug level for the parser
 * 
 * Controls the verbosity of parser-related logging:
 * - 0: Minimal logging
 * - 1: Basic parsing info
 * - 2: Detailed parsing info
 * - 3: Verbose debugging output
 * 
 * @param level New debug level (0-3)
 */
void parser_set_debug_level(int level);

/**
 * @brief Gets the current debug level of the parser
 * 
 * @return int Current debug level (0-3)
 */
int parser_get_debug_level(void);

/**
 * @brief Gets parser statistics
 * 
 * Retrieves various statistics about the parsing process, including:
 * - Number of AST nodes created
 * - Number of syntax errors encountered
 * 
 * @param nodes_created Pointer to store the number of nodes created
 * @param errors_found Pointer to store the number of errors found
 */
void parser_get_stats(int* nodes_created, int* errors_found);

/**
 * @brief Frees the AST and all its nodes
 * 
 * Recursively frees all memory allocated for the AST nodes,
 * including their associated data structures.
 * 
 * @param root Pointer to the root node of the AST to free
 */
void freeAst(AstNode *root);

/**
 * @brief Advances to the next token in the input stream
 * 
 * Simple wrapper around getNextToken() for token advancement.
 * Updates the internal currentToken state.
 */
void nextToken(void);

/**
 * @brief Expects a specific token type and advances to the next token
 * 
 * @param tokenType The expected token type
 * @throws parserError if the current token doesn't match the expected type
 */
void expectToken(int tokenType);

/**
 * @brief Parses a block of statements until an end token
 * 
 * @param count Pointer to store the number of statements parsed
 * @return AstNode** Array of parsed statement nodes
 */
AstNode** parseBlock(int* count);

#endif /* PARSER_H */
