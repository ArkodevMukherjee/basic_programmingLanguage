#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_EQUALS,
    TOKEN_PLUS,
    TOKEN_PRINT,
    TOKEN_EOF,
    TOKEN_NEWLINE
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
    int value;
} Token;

Token tokens[256];
int token_count = 0;

void tokenize(const char *source) {
    const char *p = source;
    while (*p != '\0') {
        if (isspace(*p)) {
            if (*p == '\n') {
                tokens[token_count++] = (Token){TOKEN_NEWLINE, "\n", 0};
            }
            p++;
            continue;
        } else if (isdigit(*p)) {
            Token token;
            token.type = TOKEN_NUMBER;
            token.value = strtol(p, (char **)&p, 10);
            sprintf(token.lexeme, "%d", token.value);
            tokens[token_count++] = token;
            continue;
        } else if (isalpha(*p)) {
            Token token;
            if (strncmp(p, "print", 5) == 0) {
                token.type = TOKEN_PRINT;
                strcpy(token.lexeme, "print");
                p += 5;
            } else {
                token.type = TOKEN_IDENTIFIER;
                int i = 0;
                while (isalnum(*p) && i < 63)
                    token.lexeme[i++] = *p++;
                token.lexeme[i] = '\0';
            }
            tokens[token_count++] = token;
            continue;
        } else if (*p == '=') {
            tokens[token_count++] = (Token){TOKEN_EQUALS, "=", 0};
            p++;
        } else if (*p == '+') {
            tokens[token_count++] = (Token){TOKEN_PLUS, "+", 0};
            p++;
        } else {
            fprintf(stderr, "Unexpected character: %c\n", *p);
            exit(1);
        }
    }
    tokens[token_count++] = (Token){TOKEN_EOF, "EOF", 0};
}

// AST Node Types
typedef enum {
    AST_ASSIGN,
    AST_PRINT,
    AST_BINARY_OP,
    AST_NUMBER,
    AST_VARIABLE
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        struct {
            char name[64];
            struct ASTNode *value;
        } assign;
        struct {
            struct ASTNode *expr;
        } print;
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        int number;
        char variable[64];
    };
} ASTNode;

ASTNode *new_number(int value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_NUMBER;
    node->number = value;
    return node;
}

ASTNode *new_variable(char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_VARIABLE;
    strcpy(node->variable, name);
    return node;
}

ASTNode *new_binary_op(ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BINARY_OP;
    node->binary_op.left = left;
    node->binary_op.right = right;
    return node;
}

ASTNode *new_assign(char *name, ASTNode *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    strcpy(node->assign.name, name);
    node->assign.value = value;
    return node;
}

ASTNode *new_print(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_PRINT;
    node->print.expr = expr;
    return node;
}

// Parser Helpers
int current_token = 0;

Token *get_current_token() {
    return &tokens[current_token];
}

void advance_token() {
    if (tokens[current_token].type != TOKEN_EOF) {
        current_token++;
    }
}

int match(TokenType type) {
    if (get_current_token()->type == type) {
        advance_token();
        return 1;
    }
    return 0;
}

ASTNode *parse_term() {
    Token *token = get_current_token();

    if (token->type == TOKEN_NUMBER) {
        ASTNode *node = new_number(token->value);
        advance_token();
        return node;
    } else if (token->type == TOKEN_IDENTIFIER) {
        ASTNode *node = new_variable(token->lexeme);
        advance_token();
        return node;
    }

    fprintf(stderr, "Unexpected token: %s\n", token->lexeme);
    exit(1);
}

ASTNode *parse_expr() {
    ASTNode *left = parse_term();

    while (match(TOKEN_PLUS)) {
        ASTNode *right = parse_term();
        left = new_binary_op(left, right);
    }

    return left;
}

ASTNode *parse_assign() {
    Token *identifier_token = get_current_token();
    if (identifier_token->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier\n");
        exit(1);
    }
    char name[64];
    strcpy(name, identifier_token->lexeme);
    advance_token();

    if (!match(TOKEN_EQUALS)) {
        fprintf(stderr, "Expected '=' after identifier\n");
        exit(1);
    }

    ASTNode *value = parse_expr();
    return new_assign(name, value);
}

ASTNode *parse_print() {
    if (!match(TOKEN_PRINT)) {
        fprintf(stderr, "Expected 'print'\n");
        exit(1);
    }

    ASTNode *expr = parse_expr();
    return new_print(expr);
}

ASTNode *parse_stmt() {
    while (get_current_token()->type == TOKEN_NEWLINE) {
        advance_token();
    }

    Token *token = get_current_token();

    if (token->type == TOKEN_IDENTIFIER) {
        return parse_assign();
    } else if (token->type == TOKEN_PRINT) {
        return parse_print();
    } else {
        fprintf(stderr, "Unexpected token: %s\n", token->lexeme);
        exit(1);
    }
}

// Evaluation
typedef struct {
    char name[64];
    int value;
} Variable;

Variable variables[256];
int variable_count = 0;

int eval(ASTNode *node) {
    if (node->type == AST_NUMBER) {
        return node->number;
    } else if (node->type == AST_VARIABLE) {
        for (int i = 0; i < variable_count; i++) {
            if (strcmp(variables[i].name, node->variable) == 0) {
                return variables[i].value;
            }
        }
        fprintf(stderr, "Undefined variable: %s\n", node->variable);
        exit(1);
    } else if (node->type == AST_BINARY_OP) {
        return eval(node->binary_op.left) + eval(node->binary_op.right);
    } else if (node->type == AST_ASSIGN) {
        int value = eval(node->assign.value);
        for (int i = 0; i < variable_count; i++) {
            if (strcmp(variables[i].name, node->assign.name) == 0) {
                variables[i].value = value;
                return value;
            }
        }
        strcpy(variables[variable_count].name, node->assign.name);
        variables[variable_count].value = value;
        variable_count++;
        return value;
    } else if (node->type == AST_PRINT) {
        int value = eval(node->print.expr);
        printf("%d\n", value);
        return value;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char source_code[1024];
    size_t len = fread(source_code, 1, sizeof(source_code) - 1, file);
    fclose(file);
    source_code[len] = '\0';

    tokenize(source_code);

    while (get_current_token()->type != TOKEN_EOF) {
        if (get_current_token()->type != TOKEN_NEWLINE) {
            ASTNode *stmt = parse_stmt();
            eval(stmt);
        } else {
            advance_token();
        }
    }

    return 0;
}