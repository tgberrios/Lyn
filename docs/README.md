# Lyn – High-Performance and Versatile Language

Lyn is a modern programming language that combines Python's readability with static typing and advanced features from languages like C++, JavaScript, and AspectJ. Its compilation to C code enables high performance and excellent portability.

## Main Features

- **Clean and expressive syntax**: Designed to be easy to read and write
- **Static typing with inference**: Type safety without excessive syntax
- **Object-oriented**: Full support for classes and inheritance
- **First-class functions**: Lambdas and higher-order functions
- **Robust error handling**: Try-catch-finally exception system
- **Aspect-oriented programming**: Supports aspects, pointcuts, and advice
- **C compilation**: Optimal performance and compatibility with existing code

## Current Project Status

### Implemented Features ✅

1. **Type System**

   - Primitive types (int, float, string, bool)
   - Type inference
   - Static type checking
   - Type compatibility

2. **Control Structures**

   - If-else and switch
   - While, do-while, and for loops
   - Break and continue
   - Basic pattern matching

3. **Object-Oriented Programming**

   - Classes and objects
   - Methods and constructors
   - Attributes and encapsulation
   - Basic inheritance

4. **Aspect-Oriented Programming**

   - Aspects and pointcuts
   - Advice (before, after, around)
   - Aspect weaving
   - Method interception

5. **Memory System**
   - Automatic memory management
   - Optional garbage collection
   - Embedded mode with memory pooling
   - Manual memory control

### Features in Development 🚧

1. **Advanced Features**

   - Multiple inheritance
   - Interfaces and traits
   - Generics and templates
   - Advanced macros

2. **Interoperability**

   - JavaScript integration
   - Python bindings
   - NPM support
   - C/C++ FFI

3. **Module System**
   - Module importing
   - Dependency management
   - Namespaces
   - Standard modules

### Planned Features 📅

1. **Parallelization**

   - Threads and processes
   - SIMD and vectorization
   - Asynchronous programming
   - Coroutines

2. **Ecosystem**
   - Package manager (lyn_pm)
   - Central repository (lyn_hub)
   - Development tools
   - Integrated debugger

## Code Examples

### Basic Syntax

```lyn
// Hello World
print("Hello, World!")

// Variables and types
x: int = 10
y = 20  // Type inference
text = "Hello, Lyn!"

// Functions
func add(a: int, b: int) -> int
    return a + b;
end

// Classes
class Person
    name: string
    age: int

    func init(name: string, age: int)
        this.name = name
        this.age = age
    end
end

// Simple loop
for i in range(1, 6)
    print(i)
end

// If statement
if (age >= 18)
    print("You are an adult")
else
    print("You are not yet an adult")
end
```

### Advanced Features

```lyn
// Pattern Matching
match value
    when 0:
        print("Zero")
    when n if n > 0:
        print("Positive")
    otherwise:
        print("Other case")
end

// Exception Handling
try
    result = divide(a, b)
catch (error)
    print("Error: " + error.message)
end

// Function Composition
composed = add_one >> multiply_by_two
result = composed(5)
```

## Documentation

- [Technical Documentation](docs.md): Technical details and specifications
- [Code Examples](examples.md): Practical guide with examples
- [TODO](TODO.md): Detailed feature status

## System Requirements

- C compiler (gcc/clang)
- Compatible operating system (Linux, macOS, Windows)
- 4GB RAM minimum
- 1GB disk space

## Installation

You can install Lyn in two ways:

### Using build.sh (Recommended)

```bash
# Clone the repository
git clone https://github.com/your-username/lyn.git
cd lyn

# Run the build script
./build.sh
```

### Manual Installation

```bash
git clone https://github.com/your-username/lyn.git
cd lyn
make
sudo make install
```

## Basic Usage

```bash
# Compile a file
lync program.lyn

# Run the program
./program

# Compile with optimizations
lync -O3 program.lyn

# Embedded mode
lync --embedded program.lyn
```

## Quick Start Guide

1. Create a new file `hello.lyn`:

```lyn
main
    print("Hello, World!")
end
```

2. Compile and run:

```bash
lync hello.lyn
./hello
```

3. For more complex projects, use the build script:

```bash
./build.sh
```

The build script will handle all necessary dependencies and compilation steps automatically.
