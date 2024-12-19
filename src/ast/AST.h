#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Estructura para almacenar la ubicación en el código fuente
struct SourceLocation {
    int line;
    int column;
};

// Declaración adelantada para el Patrón Visitor
class ASTVisitor;

// CLASE AST QUE REPRESENTA EL AST DE UN PROGRAMA
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Método virtual para aceptar un visitante
    virtual void accept(ASTVisitor& visitor) = 0;

protected:
    SourceLocation location;
};

// Definición de la clase ASTVisitor
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Métodos visit para cada tipo de nodo
    virtual void visit(class LiteralNode& node) = 0;
    virtual void visit(class VariableNode& node) = 0;
    virtual void visit(class BinaryOperationNode& node) = 0;
    virtual void visit(class UnaryOperationNode& node) = 0;
    virtual void visit(class AssignmentNode& node) = 0;
    virtual void visit(class IfNode& node) = 0;
    virtual void visit(class ForNode& node) = 0;
    virtual void visit(class WhileNode& node) = 0;
    virtual void visit(class DoWhileNode& node) = 0;
    virtual void visit(class SwitchNode& node) = 0;
    virtual void visit(class CaseNode& node) = 0;
    virtual void visit(class DefaultNode& node) = 0;
    virtual void visit(class BreakNode& node) = 0;
    virtual void visit(class ContinueNode& node) = 0;
    virtual void visit(class ReturnNode& node) = 0;
    virtual void visit(class FunctionDeclarationNode& node) = 0;
    virtual void visit(class FunctionCallNode& node) = 0;
    virtual void visit(class ClassDeclarationNode& node) = 0;
    virtual void visit(class ClassMethodNode& node) = 0;
    virtual void visit(class ClassPropertyNode& node) = 0;
    virtual void visit(class ClassPropertyGetterNode& node) = 0;
    virtual void visit(class ClassPropertySetterNode& node) = 0;
    virtual void visit(class ClassConstructorNode& node) = 0;
    virtual void visit(class ClassConstructorMethodNode& node) = 0;
    virtual void visit(class MethodDeclarationNode& node) = 0;
    virtual void visit(class PropertyNode& node) = 0;
    virtual void visit(class NamespaceNode& node) = 0;
    virtual void visit(class ImportNode& node) = 0;
    virtual void visit(class ExportNode& node) = 0;
    virtual void visit(class BlockNode& node) = 0;
    virtual void visit(class PrintNode& node) = 0;
};

// Nodo para literales
class LiteralNode : public ASTNode {
public:
    enum class Type {
        INT,            // Representa números enteros
        FLOAT,          // Representa números de punto flotante
        BOOLEAN,        // Representa `true` o `false`
        STRING,         // Representa cadenas de texto
        NULL_VALUE,     // Representa `null`
        UNDEFINED_VALUE // Representa `undefined`
    };

    LiteralNode(Type type, const std::string& value, const SourceLocation& loc = {0, 0})
        : type(type), value(value) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Type type;          
    std::string value;  
};

// Nodo para variables
class VariableNode : public ASTNode {
public:
    enum class Type {
        IDENTIFIER, // Representa una variable declarada con `var`
        CONSTANT    // Representa una variable declarada con `const`
    };

    VariableNode(Type type, const std::string& name, const SourceLocation& loc = {0, 0})
        : type(type), name(name) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Type type;           // Tipo de la variable
    std::string name;    // Nombre de la variable
};

// Nodo para operaciones binarias
class BinaryOperationNode : public ASTNode { 
public:
    enum class Type {
        ADDITION,       // Suma
        SUBTRACTION,    // Resta  
        MULTIPLICATION, // Multiplicación
        DIVISION,       // División
        EQUAL,          // Igualdad
        NOT_EQUAL,      // No igualdad
        LESS_THAN,      // Menor que
        GREATER_THAN,   // Mayor que
        LESS_EQUAL,     // Menor o igual que
        GREATER_EQUAL,  // Mayor o igual que
        AND,            // AND
        OR,             // OR
        BITWISE_AND,    // AND bit a bit
        BITWISE_OR,     // OR bit a bit
        BITWISE_XOR,    // XOR bit a bit
        ASSIGN,         // Asignación
        ASSIGN_PLUS,    // Asignación suma
        ASSIGN_MINUS,   // Asignación resta
        ASSIGN_MULTIPLY,// Asignación multiplicación
        ASSIGN_DIVIDE   // Asignación división
    };

    BinaryOperationNode(Type type, std::unique_ptr<ASTNode> left, 
                       std::unique_ptr<ASTNode> right, const SourceLocation& loc = {0, 0})
        : type(type), left(std::move(left)), right(std::move(right)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Type type;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

// Nodo para operaciones unarias
class UnaryOperationNode : public ASTNode { 
public:
    enum class Type {
        PLUS,           // +
        MINUS,          // -
        NOT,            // !
        BITWISE_NOT     // ~
    };

    UnaryOperationNode(Type type, std::unique_ptr<ASTNode> operand, const SourceLocation& loc = {0, 0})
        : type(type), operand(std::move(operand)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Type type;
    std::unique_ptr<ASTNode> operand;
};

// Nodo para asignaciones
class AssignmentNode : public ASTNode { 
public:
    enum class Type {
        ASSIGN,         // Asignación
        ASSIGN_PLUS,    // Asignación suma
        ASSIGN_MINUS,   // Asignación resta
        ASSIGN_MULTIPLY,// Asignación multiplicación
        ASSIGN_DIVIDE   // Asignación división
    };

    AssignmentNode(Type type, std::unique_ptr<ASTNode> target, 
                   std::unique_ptr<ASTNode> value, const SourceLocation& loc = {0, 0})
        : type(type), target(std::move(target)), value(std::move(value)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Type type;
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> value;
};

// Nodo para estructuras de control If
class IfNode : public ASTNode {
public:
    IfNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, 
           std::unique_ptr<ASTNode> elseBranch = nullptr, const SourceLocation& loc = {0, 0})
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
};

// Nodo para bucles For
class ForNode : public ASTNode {
public:
    ForNode(std::unique_ptr<ASTNode> initialization, std::unique_ptr<ASTNode> condition, 
            std::unique_ptr<ASTNode> increment, std::unique_ptr<ASTNode> body, 
            const SourceLocation& loc = {0, 0})
        : initialization(std::move(initialization)), condition(std::move(condition)), 
          increment(std::move(increment)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> initialization;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> increment;
    std::unique_ptr<ASTNode> body;
};

// Nodo para bucles While
class WhileNode : public ASTNode {
public:
    WhileNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
              const SourceLocation& loc = {0, 0})
        : condition(std::move(condition)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
};

// Nodo para bucles Do-While
class DoWhileNode : public ASTNode {
public:
    DoWhileNode(std::unique_ptr<ASTNode> body, std::unique_ptr<ASTNode> condition, 
               const SourceLocation& loc = {0, 0})
        : body(std::move(body)), condition(std::move(condition)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> body;
    std::unique_ptr<ASTNode> condition;
};

// Nodo para estructuras Switch
class SwitchNode : public ASTNode {
public:
    SwitchNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
               const SourceLocation& loc = {0, 0})
        : condition(std::move(condition)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
};

// Nodo para casos en Switch
class CaseNode : public ASTNode {
public:
    CaseNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
             const SourceLocation& loc = {0, 0})
        : condition(std::move(condition)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
};

// Nodo para el caso por defecto en Switch
class DefaultNode : public ASTNode {
public:
    DefaultNode(std::unique_ptr<ASTNode> body, const SourceLocation& loc = {0, 0})
        : body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> body;
};

// Nodo para la instrucción Break
class BreakNode : public ASTNode {
public:
    BreakNode(const SourceLocation& loc = {0, 0}) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

// Nodo para la instrucción Continue
class ContinueNode : public ASTNode {
public:
    ContinueNode(const SourceLocation& loc = {0, 0}) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }
};

// Nodo para la instrucción Return
class ReturnNode : public ASTNode {
public:
    ReturnNode(std::unique_ptr<ASTNode> value, const SourceLocation& loc = {0, 0})
        : value(std::move(value)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> value;
};

// Nodo para declaraciones de funciones
class FunctionDeclarationNode : public ASTNode {
public:
    FunctionDeclarationNode(const std::string& name, 
                            std::vector<std::unique_ptr<ASTNode>> parameters, 
                            std::unique_ptr<ASTNode> body, 
                            const SourceLocation& loc = {0, 0})
        : name(name), parameters(std::move(parameters)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
};

// Nodo para llamadas a funciones
class FunctionCallNode : public ASTNode {
public:
    FunctionCallNode(const std::string& name, 
                     std::vector<std::unique_ptr<ASTNode>> arguments, 
                     const SourceLocation& loc = {0, 0})
        : name(name), arguments(std::move(arguments)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
};

// Nodo para declaraciones de clases
class ClassDeclarationNode : public ASTNode {
public:
    ClassDeclarationNode(const std::string& name, 
                         std::vector<std::string> superclass, 
                         std::vector<std::string> interfaces, 
                         std::unique_ptr<ASTNode> body, 
                         const SourceLocation& loc = {0, 0})
        : name(name), superclass(std::move(superclass)), 
          interfaces(std::move(interfaces)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::string> superclass;
    std::vector<std::string> interfaces;
    std::unique_ptr<ASTNode> body;
};

// Nodo para métodos de clases
class ClassMethodNode : public ASTNode {
public:
    ClassMethodNode(const std::string& name, 
                    std::vector<std::unique_ptr<ASTNode>> parameters, 
                    std::unique_ptr<ASTNode> body, 
                    const SourceLocation& loc = {0, 0})
        : name(name), parameters(std::move(parameters)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
};

// Nodo para propiedades de clases
class ClassPropertyNode : public ASTNode {
public:
    ClassPropertyNode(const std::string& name, 
                      const std::string& type, 
                      const SourceLocation& loc = {0, 0})
        : name(name), type(type) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::string type;
};

// Nodo para getters de propiedades de clases
class ClassPropertyGetterNode : public ASTNode {
public:
    ClassPropertyGetterNode(const std::string& name, 
                            const std::string& type, 
                            std::unique_ptr<ASTNode> body, 
                            const SourceLocation& loc = {0, 0})
        : name(name), type(type), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> body;
};

// Nodo para setters de propiedades de clases
class ClassPropertySetterNode : public ASTNode {
public:
    ClassPropertySetterNode(const std::string& name, 
                            const std::string& type, 
                            std::unique_ptr<ASTNode> parameter, 
                            std::unique_ptr<ASTNode> body, 
                            const SourceLocation& loc = {0, 0})
        : name(name), type(type), parameter(std::move(parameter)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> parameter;
    std::unique_ptr<ASTNode> body;
};

// Nodo para constructores de clases
class ClassConstructorNode : public ASTNode {
public:
    ClassConstructorNode(const std::string& name, 
                         std::vector<std::unique_ptr<ASTNode>> parameters, 
                         std::unique_ptr<ASTNode> body, 
                         const SourceLocation& loc = {0, 0})
        : name(name), parameters(std::move(parameters)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
};

// Nodo para métodos de constructores de clases (si es necesario)
class ClassConstructorMethodNode : public ASTNode {
public:
    ClassConstructorMethodNode(const std::string& name, 
                                std::vector<std::unique_ptr<ASTNode>> parameters, 
                                std::unique_ptr<ASTNode> body, 
                                const SourceLocation& loc = {0, 0})
        : name(name), parameters(std::move(parameters)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
};

// Nodo para declaraciones de métodos
class MethodDeclarationNode : public ASTNode {
public:
    MethodDeclarationNode(const std::string& name, 
                          std::vector<std::unique_ptr<ASTNode>> parameters, 
                          std::unique_ptr<ASTNode> body, 
                          const SourceLocation& loc = {0, 0})
        : name(name), parameters(std::move(parameters)), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
};

// Nodo para propiedades generales
class PropertyNode : public ASTNode {
public:
    PropertyNode(const std::string& name, 
                const std::string& type, 
                const SourceLocation& loc = {0, 0})
        : name(name), type(type) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::string type;
};

// Nodo para espacios de nombres (namespaces)
class NamespaceNode : public ASTNode {
public:
    NamespaceNode(const std::string& name, 
                 std::unique_ptr<ASTNode> body, 
                 const SourceLocation& loc = {0, 0})
        : name(name), body(std::move(body)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
    std::unique_ptr<ASTNode> body;
};

// Nodo para importaciones
class ImportNode : public ASTNode {
public:
    ImportNode(const std::string& name, const SourceLocation& loc = {0, 0})
        : name(name) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
};

// Nodo para exportaciones
class ExportNode : public ASTNode {
public:
    ExportNode(const std::string& name, const SourceLocation& loc = {0, 0})
        : name(name) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::string name;
};

// Nodo para bloques de código
class BlockNode : public ASTNode {
public:
    BlockNode(std::vector<std::unique_ptr<ASTNode>> statements, 
              const SourceLocation& loc = {0, 0})
        : statements(std::move(statements)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<ASTNode>> statements;
};

// Nodo para impresión (print)
class PrintNode : public ASTNode {
public:
    PrintNode(std::unique_ptr<ASTNode> value, const SourceLocation& loc = {0, 0})
        : value(std::move(value)) {
        location = loc;
    }

    void accept(ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::unique_ptr<ASTNode> value;
};

#endif // AST_H
