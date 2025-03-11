#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer.h"
#include "../parser.h"
#include "../compiler.h"
#include "../error.h"

#define TEST_PASS printf("✅ Test passed: %s\n", __func__)
#define TEST_FAIL printf("❌ Test failed: %s\n", __func__)
#define ASSERT(condition) do { if (!(condition)) { TEST_FAIL; return 0; } } while(0)

// Test para la clase Point
static int test_point_class() {
    const char* source = 
        "main\n"
        "    class Point\n"
        "        x: float;\n"
        "        y: float;\n"
        "        func init(self: Point, x: float, y: float) -> void\n"
        "            self.x = x;\n"
        "            self.y = y;\n"
        "        end\n"
        "        func distance(self: Point, other: Point) -> float\n"
        "            dx = self.x - other.x;\n"
        "            dy = self.y - other.y;\n"
        "            return sqrt(dx * dx + dy * dy);\n"
        "        end\n"
        "    end\n"
        "end\n";

    lexerInit(source);
    error_init();
    
    AstNode* ast = parseProgram();
    ASSERT(ast != NULL);
    ASSERT(ast->type == AST_PROGRAM);
    ASSERT(ast->program.statementCount > 0);
    
    // Verificar que la primera declaración es una clase Point
    AstNode* classNode = ast->program.statements[0];
    ASSERT(classNode->type == AST_CLASS_DEF);
    ASSERT(strcmp(classNode->classDef.name, "Point") == 0);

    freeAst(ast);
    TEST_PASS;
    return 1;
}

// Test para la clase Vector3
static int test_vector3_class() {
    const char* source = 
        "main\n"
        "    class Vector3\n"
        "        x: float;\n"
        "        y: float;\n"
        "        z: float;\n"
        "        func init(self: Vector3, x: float, y: float, z: float) -> void\n"
        "            self.x = x;\n"
        "            self.y = y;\n"
        "            self.z = z;\n"
        "        end\n"
        "    end\n"
        "end\n";

    lexerInit(source);
    error_init();
    
    AstNode* ast = parseProgram();
    ASSERT(ast != NULL);
    ASSERT(ast->type == AST_PROGRAM);
    
    // Verificar la clase Vector3
    AstNode* classNode = ast->program.statements[0];
    ASSERT(classNode->type == AST_CLASS_DEF);
    ASSERT(strcmp(classNode->classDef.name, "Vector3") == 0);

    freeAst(ast);
    TEST_PASS;
    return 1;
}

// Test para herencia Circle:Shape
static int test_inheritance() {
    const char* source = 
        "main\n"
        "    class Shape\n"
        "        x: float;\n"
        "        y: float;\n"
        "    end\n"
        "    class Circle : Shape\n"
        "        radius: float;\n"
        "    end\n"
        "end\n";

    lexerInit(source);
    error_init();
    
    AstNode* ast = parseProgram();
    ASSERT(ast != NULL);
    ASSERT(ast->type == AST_PROGRAM);
    
    // Verificar la herencia
    AstNode* circleNode = ast->program.statements[1];
    ASSERT(circleNode->type == AST_CLASS_DEF);
    ASSERT(strcmp(circleNode->classDef.name, "Circle") == 0);
    ASSERT(strcmp(circleNode->classDef.baseClassName, "Shape") == 0);

    freeAst(ast);
    TEST_PASS;
    return 1;
}

int main() {
    printf("\nRunning compiler tests...\n");
    printf("========================\n\n");

    int passed = 0;
    int total = 0;

    // Ejecutar tests
    total++; passed += test_point_class();
    total++; passed += test_vector3_class();
    total++; passed += test_inheritance();

    // Mostrar resumen
    printf("\n========================\n");
    printf("Tests passed: %d/%d\n", passed, total);
    printf("========================\n");

    return passed == total ? 0 : 1;
}
