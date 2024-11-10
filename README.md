# Simple Expression Compiler

This project is a basic compiler written in C that processes a custom script with basic arithmetic operations, variable assignment, and print statements. The compiler reads source code from a file, tokenizes the input, builds an Abstract Syntax Tree (AST), and evaluates the statements to produce output.

## Features

- **Tokenization**: Breaks down source code into tokens for parsing.
- **AST Construction**: Builds an AST from tokens to represent expressions and statements.
- **Evaluation**: Evaluates arithmetic expressions, assignments, and print statements.
- **Error Handling**: Basic error handling for undefined variables, unexpected tokens, and syntax issues.

## Supported Syntax

The compiler currently supports:
- Variable assignment, e.g., `x = 10`
- Arithmetic operations with `+`, e.g., `x + 5`
- Print statements, e.g., `print x + 5`

### Example Script

```plaintext
x = 10
print x + 5
