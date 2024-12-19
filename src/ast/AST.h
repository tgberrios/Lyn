// AST.h
#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Forward declarations de todas las clases de nodos AST
class LiteralNode;
class VariableNode;
class BinaryOperationNode;
class UnaryOperationNode;
class AssignmentNode;
class IfNode;
class ForNode;
class WhileNode;
class DoWhileNode;
class SwitchNode;
class CaseNode;
class DefaultNode;
class BreakNode;
class ContinueNode;
class ReturnNode;
class FunctionDeclarationNode;
class FunctionCallNode;
class ClassDeclarationNode;
class ClassMethodNode;
class ClassPropertyNode;
class ClassPropertyGetterNode;
class ClassPropertySetterNode;
class ClassConstructorNode;
class ClassConstructorMethodNode;
class MethodDeclarationNode;
class PropertyNode;
class NamespaceNode;
class ImportNode;
class ExportNode;
class BlockNode;
class PrintNode;
class VariableDeclarationNode;
class ExpressionStatementNode;

// CLASE AST QUE REPRESENTA EL AST DE UN PROGRAMA
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Método virtual para aceptar un visitante
    virtual void accept(class ASTVisitor& visitor) = 0;
};

// Definición de todas las clases AST derivadas

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

    LiteralNode(Type type, const std::string& value, int line = 0, int column = 0)
        : type(type), value(value), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    Type type;          
    std::string value;  
    int line;
    int column;
};

// Nodo para variables
class VariableNode : public ASTNode {
public:
    enum class Type {
        IDENTIFIER, // Representa una variable declarada con `var`
        CONSTANT    // Representa una variable declarada con `const`
    };

    VariableNode(Type type, const std::string& name, int line = 0, int column = 0)
        : type(type), name(name), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    Type type;           // Tipo de la variable
    std::string name;    // Nombre de la variable
    int line;
    int column;
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
        AND,            // AND lógico
        OR,             // OR lógico
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
                       std::unique_ptr<ASTNode> right, int line = 0, int column = 0)
        : type(type), left(std::move(left)), right(std::move(right)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    Type type;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    int line;
    int column;
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

    UnaryOperationNode(Type type, std::unique_ptr<ASTNode> operand, int line = 0, int column = 0)
        : type(type), operand(std::move(operand)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    Type type;
    std::unique_ptr<ASTNode> operand;
    int line;
    int column;
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
                   std::unique_ptr<ASTNode> value, int line = 0, int column = 0)
        : type(type), target(std::move(target)), value(std::move(value)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    Type type;
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> value;
    int line;
    int column;
};

// Nodo para declaraciones de variables
class VariableDeclarationNode : public ASTNode {
public:
    VariableDeclarationNode(std::vector<std::unique_ptr<ASTNode>> vars, int line = 0, int column = 0)
        : variables(std::move(vars)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::vector<std::unique_ptr<ASTNode>> variables;
    int line;
    int column;
};

// Nodo para expresiones usadas como sentencias
class ExpressionStatementNode : public ASTNode {
public:
    ExpressionStatementNode(std::unique_ptr<ASTNode> expr, int line = 0, int column = 0)
        : expression(std::move(expr)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> expression;
    int line;
    int column;
};

// Nodo para estructuras de control If
class IfNode : public ASTNode {
public:
    IfNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, 
           std::unique_ptr<ASTNode> elseBranch = nullptr, int line = 0, int column = 0)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
    int line;
    int column;
};

// Nodo para bucles For
class ForNode : public ASTNode {
public:
    ForNode(std::unique_ptr<ASTNode> initialization, std::unique_ptr<ASTNode> condition, 
            std::unique_ptr<ASTNode> increment, std::unique_ptr<ASTNode> body, 
            int line = 0, int column = 0)
        : initialization(std::move(initialization)), condition(std::move(condition)), 
          increment(std::move(increment)), body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> initialization;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> increment;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para bucles While
class WhileNode : public ASTNode {
public:
    WhileNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
              int line = 0, int column = 0)
        : condition(std::move(condition)), body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para bucles Do-While
class DoWhileNode : public ASTNode {
public:
    DoWhileNode(std::unique_ptr<ASTNode> body, std::unique_ptr<ASTNode> condition, 
               int line = 0, int column = 0)
        : body(std::move(body)), condition(std::move(condition)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> body;
    std::unique_ptr<ASTNode> condition;
    int line;
    int column;
};

// Nodo para estructuras Switch
class SwitchNode : public ASTNode {
public:
    SwitchNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
               int line = 0, int column = 0)
        : condition(std::move(condition)), body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para casos en Switch
class CaseNode : public ASTNode {
public:
    CaseNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, 
             int line = 0, int column = 0)
        : condition(std::move(condition)), body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para el caso por defecto en Switch
class DefaultNode : public ASTNode {
public:
    DefaultNode(std::unique_ptr<ASTNode> body, int line = 0, int column = 0)
        : body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para la instrucción Break
class BreakNode : public ASTNode {
public:
    BreakNode(int line = 0, int column = 0)
        : line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    int line;
    int column;
};

// Nodo para la instrucción Continue
class ContinueNode : public ASTNode {
public:
    ContinueNode(int line = 0, int column = 0)
        : line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    int line;
    int column;
};

// Nodo para la instrucción Return
class ReturnNode : public ASTNode {
public:
    ReturnNode(std::unique_ptr<ASTNode> value, int line = 0, int column = 0)
        : value(std::move(value)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> value;
    int line;
    int column;
};

// Nodo para declaraciones de funciones
class FunctionDeclarationNode : public ASTNode {
public:
    FunctionDeclarationNode(const std::string& name, 
                            std::vector<std::unique_ptr<ASTNode>> parameters, 
                            std::unique_ptr<ASTNode> body, 
                            int line = 0, int column = 0)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para llamadas a funciones
class FunctionCallNode : public ASTNode {
public:
    FunctionCallNode(const std::string& name, 
                     std::vector<std::unique_ptr<ASTNode>> arguments, 
                     int line = 0, int column = 0)
        : name(name), arguments(std::move(arguments)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    int line;
    int column;
};

// Nodo para declaraciones de clases
class ClassDeclarationNode : public ASTNode {
public:
    ClassDeclarationNode(const std::string& name, 
                         std::string superclass, 
                         std::vector<std::unique_ptr<ASTNode>> interfaces, 
                         std::unique_ptr<ASTNode> body, 
                         int line = 0, int column = 0)
        : name(name), superclass(std::move(superclass)), interfaces(std::move(interfaces)), 
          body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::string superclass;
    std::vector<std::unique_ptr<ASTNode>> interfaces;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para métodos de clases
class ClassMethodNode : public ASTNode {
public:
    ClassMethodNode(const std::string& name, 
                    std::vector<std::unique_ptr<ASTNode>> parameters, 
                    std::unique_ptr<ASTNode> body, 
                    int line = 0, int column = 0)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para propiedades de clases
class ClassPropertyNode : public ASTNode {
public:
    ClassPropertyNode(const std::string& name, 
                      std::string type, 
                      std::unique_ptr<ASTNode> initializer, 
                      int line = 0, int column = 0)
        : name(name), type(std::move(type)), initializer(std::move(initializer)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> initializer;
    int line;
    int column;
};

// Nodo para getters de propiedades de clases
class ClassPropertyGetterNode : public ASTNode {
public:
    ClassPropertyGetterNode(const std::string& name, 
                            std::string type, 
                            std::unique_ptr<ASTNode> body, 
                            int line = 0, int column = 0)
        : name(name), type(std::move(type)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para setters de propiedades de clases
class ClassPropertySetterNode : public ASTNode {
public:
    ClassPropertySetterNode(const std::string& name, 
                            std::string type, 
                            std::unique_ptr<ASTNode> parameter, 
                            std::unique_ptr<ASTNode> body, 
                            int line = 0, int column = 0)
        : name(name), type(std::move(type)), parameter(std::move(parameter)), 
          body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> parameter;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para constructores de clases
class ClassConstructorNode : public ASTNode {
public:
    ClassConstructorNode(const std::string& name, 
                         std::vector<std::unique_ptr<ASTNode>> parameters, 
                         std::unique_ptr<ASTNode> body, 
                         int line = 0, int column = 0)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para métodos de constructores de clases (si es necesario)
class ClassConstructorMethodNode : public ASTNode {
public:
    ClassConstructorMethodNode(const std::string& name, 
                                std::vector<std::unique_ptr<ASTNode>> parameters, 
                                std::unique_ptr<ASTNode> body, 
                                int line = 0, int column = 0)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para declaraciones de métodos
class MethodDeclarationNode : public ASTNode {
public:
    MethodDeclarationNode(const std::string& name, 
                          std::vector<std::unique_ptr<ASTNode>> parameters, 
                          std::unique_ptr<ASTNode> body, 
                          int line = 0, int column = 0)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para propiedades generales
class PropertyNode : public ASTNode {
public:
    PropertyNode(const std::string& name, 
                std::string type, 
                std::unique_ptr<ASTNode> initializer, 
                int line = 0, int column = 0)
        : name(name), type(std::move(type)), initializer(std::move(initializer)), 
          line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::string type;
    std::unique_ptr<ASTNode> initializer;
    int line;
    int column;
};

// Nodo para espacios de nombres (namespaces)
class NamespaceNode : public ASTNode {
public:
    NamespaceNode(const std::string& name, 
                 std::unique_ptr<ASTNode> body, 
                 int line = 0, int column = 0)
        : name(name), body(std::move(body)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    std::unique_ptr<ASTNode> body;
    int line;
    int column;
};

// Nodo para importaciones
class ImportNode : public ASTNode {
public:
    ImportNode(const std::string& name, int line = 0, int column = 0)
        : name(name), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    int line;
    int column;
};

// Nodo para exportaciones
class ExportNode : public ASTNode {
public:
    ExportNode(const std::string& name, int line = 0, int column = 0)
        : name(name), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::string name;
    int line;
    int column;
};

// Nodo para bloques de código
class BlockNode : public ASTNode {
public:
    BlockNode(std::vector<std::unique_ptr<ASTNode>> statements, 
              int line = 0, int column = 0)
        : statements(std::move(statements)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::vector<std::unique_ptr<ASTNode>> statements;
    int line;
    int column;
};

// Nodo para impresión (print)
class PrintNode : public ASTNode {
public:
    PrintNode(std::unique_ptr<ASTNode> value, int line = 0, int column = 0)
        : value(std::move(value)), line(line), column(column) {}

    void accept(ASTVisitor& visitor) override;

    std::unique_ptr<ASTNode> value;
    int line;
    int column;
};

// Definición de ASTVisitor después de todas las clases AST para evitar tipos incompletos
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Métodos visit para cada tipo de nodo
    virtual void visit(LiteralNode& node) = 0;
    virtual void visit(VariableNode& node) = 0;
    virtual void visit(BinaryOperationNode& node) = 0;
    virtual void visit(UnaryOperationNode& node) = 0;
    virtual void visit(AssignmentNode& node) = 0;
    virtual void visit(IfNode& node) = 0;
    virtual void visit(ForNode& node) = 0;
    virtual void visit(WhileNode& node) = 0;
    virtual void visit(DoWhileNode& node) = 0;
    virtual void visit(SwitchNode& node) = 0;
    virtual void visit(CaseNode& node) = 0;
    virtual void visit(DefaultNode& node) = 0;
    virtual void visit(BreakNode& node) = 0;
    virtual void visit(ContinueNode& node) = 0;
    virtual void visit(ReturnNode& node) = 0;
    virtual void visit(FunctionDeclarationNode& node) = 0;
    virtual void visit(FunctionCallNode& node) = 0;
    virtual void visit(ClassDeclarationNode& node) = 0;
    virtual void visit(ClassMethodNode& node) = 0;
    virtual void visit(ClassPropertyNode& node) = 0;
    virtual void visit(ClassPropertyGetterNode& node) = 0;
    virtual void visit(ClassPropertySetterNode& node) = 0;
    virtual void visit(ClassConstructorNode& node) = 0;
    virtual void visit(ClassConstructorMethodNode& node) = 0;
    virtual void visit(MethodDeclarationNode& node) = 0;
    virtual void visit(PropertyNode& node) = 0;
    virtual void visit(NamespaceNode& node) = 0;
    virtual void visit(ImportNode& node) = 0;
    virtual void visit(ExportNode& node) = 0;
    virtual void visit(BlockNode& node) = 0;
    virtual void visit(PrintNode& node) = 0;
    virtual void visit(VariableDeclarationNode& node) = 0;
    virtual void visit(ExpressionStatementNode& node) = 0;
};

// Implementación de los métodos accept ahora que ASTVisitor está definido
inline void LiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void VariableNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void BinaryOperationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void UnaryOperationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void AssignmentNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void VariableDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ExpressionStatementNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void IfNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ForNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void WhileNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void DoWhileNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void SwitchNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void CaseNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void DefaultNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void BreakNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ContinueNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ReturnNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void FunctionDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void FunctionCallNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassMethodNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassPropertyNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassPropertyGetterNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassPropertySetterNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassConstructorNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ClassConstructorMethodNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void MethodDeclarationNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void PropertyNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void NamespaceNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ImportNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void ExportNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void BlockNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

inline void PrintNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

#endif // AST_H
