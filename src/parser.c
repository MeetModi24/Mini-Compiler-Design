#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_BINOP,     
    AST_IDENTIFIER,
    AST_NUMBER,
    AST_CONDITION,
    AST_STATEMENT_LIST
} ASTNodeType;

typedef struct AST {
    ASTNodeType type;
    char *name; 
    int value;  
    char op;    
    struct AST *left;
    struct AST *right;
    struct AST **stmts;
    int stmt_count;
    int line;
} AST;

static Token current_token;
static FILE *source_file;

void advance();
int accept(TokenType t);
int expect(TokenType t, const char *errmsg);
AST *parse_program();
AST *parse_statement();
AST *parse_declaration();
AST *parse_assignment();
AST *parse_conditional();
AST *parse_expression();
AST *parse_term();
AST *parse_condition();
AST *make_node(ASTNodeType type, int line);
void print_ast(AST *node, int indent);
void free_ast(AST *node);

void syntax_error(const char *msg) {
    fprintf(stderr, "Syntax error (line %d): %s. Got token '%s' (type %d)\n",
            current_token.line, msg, current_token.text, current_token.type);
    exit(EXIT_FAILURE);
}

void advance() {
    getNextToken(source_file, &current_token);
}

int accept(TokenType t) {
    if (current_token.type == t) {
        advance();
        return 1;
    }
    return 0;
}

int expect(TokenType t, const char *errmsg) {
    if (current_token.type == t) {
        advance();
        return 1;
    }
    syntax_error(errmsg);
    return 0;
}

AST *make_node(ASTNodeType type, int line) {
    AST *n = (AST*)malloc(sizeof(AST));
    memset(n, 0, sizeof(AST));
    n->type = type;
    n->line = line;
    return n;
}

AST *parse_program() {
    AST *program = make_node(AST_PROGRAM, current_token.line);
    AST **stmts = NULL;
    int count = 0;

    while (current_token.type != TOKEN_EOF) {
        AST *s = parse_statement();
        if (!s) break;
        stmts = (AST**)realloc(stmts, sizeof(AST*) * (count + 1));
        stmts[count++] = s;
    }

    program->stmts = stmts;
    program->stmt_count = count;
    return program;
}

AST *parse_statement() {
    if (current_token.type == TOKEN_INT) {
        return parse_declaration();
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        return parse_assignment();
    } else if (current_token.type == TOKEN_IF) {
        return parse_conditional();
    } else if (current_token.type == TOKEN_EOF) {
        return NULL;
    } else {
        syntax_error("Expected a statement (declaration, assignment, or if)");
        return NULL;
    }
}

AST *parse_declaration() {
    int ln = current_token.line;
    expect(TOKEN_INT, "Expected 'int' for declaration");
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier after 'int'");
    AST *node = make_node(AST_DECLARATION, ln);
    node->name = strdup(current_token.text);
    advance();
    expect(TOKEN_SEMICOLON, "Expected ';' after declaration");
    return node;
}

AST *parse_assignment() {
    int ln = current_token.line;
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier at assignment start");
    AST *node = make_node(AST_ASSIGNMENT, ln);
    node->name = strdup(current_token.text);
    advance();
    expect(TOKEN_ASSIGN, "Expected '=' in assignment");
    node->left = parse_expression(); 
    expect(TOKEN_SEMICOLON, "Expected ';' after assignment");
    return node;
}

AST *parse_conditional() {
    int ln = current_token.line;
    expect(TOKEN_IF, "Expected 'if'");
    expect(TOKEN_LPAREN, "Expected '(' after if");
    AST *cond = parse_condition();
    expect(TOKEN_RPAREN, "Expected ')' after condition");
    expect(TOKEN_LBRACE, "Expected '{' to start if block");

    AST **stmts = NULL;
    int count = 0;
    while (current_token.type != TOKEN_RBRACE && current_token.type != TOKEN_EOF) {
        AST *s = parse_statement();
        if (!s) break;
        stmts = (AST**)realloc(stmts, sizeof(AST*) * (count + 1));
        stmts[count++] = s;
    }
    expect(TOKEN_RBRACE, "Expected '}' to end if block");

    AST *node = make_node(AST_IF, ln);
    node->left = cond;
    node->stmts = stmts;
    node->stmt_count = count;
    return node;
}

AST *parse_condition() {
    int ln = current_token.line;
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier in condition");
    AST *left = make_node(AST_IDENTIFIER, ln);
    left->name = strdup(current_token.text);
    advance();
    expect(TOKEN_EQUAL, "Expected '==' in condition");
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier on right side of '=='");
    AST *right = make_node(AST_IDENTIFIER, current_token.line);
    right->name = strdup(current_token.text);
    advance();
    AST *cond = make_node(AST_CONDITION, ln);
    cond->left = left;
    cond->right = right;
    return cond;
}

AST *parse_expression() {
    AST *left = parse_term();
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        char op = (current_token.type == TOKEN_PLUS) ? '+' : '-';
        int ln = current_token.line;
        advance();
        AST *right = parse_term();
        AST *bin = make_node(AST_BINOP, ln);
        bin->op = op;
        bin->left = left;
        bin->right = right;
        left = bin; 
    }
    return left;
}

AST *parse_term() {
    if (current_token.type == TOKEN_IDENTIFIER) {
        AST *n = make_node(AST_IDENTIFIER, current_token.line);
        n->name = strdup(current_token.text);
        advance();
        return n;
    } else if (current_token.type == TOKEN_NUMBER) {
        AST *n = make_node(AST_NUMBER, current_token.line);
        n->value = atoi(current_token.text);
        advance();
        return n;
    } else {
        syntax_error("Expected identifier or number in expression");
        return NULL;
    }
}

void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) putchar(' ');
}

void print_ast(AST *node, int indent) {
    if (!node) return;
    print_indent(indent);
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (statements=%d)\n", node->stmt_count);
            for (int i = 0; i < node->stmt_count; ++i) {
                print_ast(node->stmts[i], indent + 2);
            }
            break;
        case AST_DECLARATION:
            printf("Decl: int %s (line %d)\n", node->name, node->line);
            break;
        case AST_ASSIGNMENT:
            printf("Assign: %s =\n", node->name);
            print_ast(node->left, indent + 2);
            break;
        case AST_IF:
            printf("If (line %d)\n", node->line);
            print_indent(indent + 2); printf("Condition:\n");
            print_ast(node->left, indent + 4);
            print_indent(indent + 2); printf("Block (statements=%d):\n", node->stmt_count);
            for (int i = 0; i < node->stmt_count; ++i) print_ast(node->stmts[i], indent + 4);
            break;
        case AST_CONDITION:
            printf("Condition: \n");
            print_ast(node->left, indent + 2);
            print_indent(indent + 2); printf("==\n");
            print_ast(node->right, indent + 2);
            break;
        case AST_BINOP:
            printf("Binop (%c)\n", node->op);
            print_ast(node->left, indent + 2);
            print_ast(node->right, indent + 2);
            break;
        case AST_IDENTIFIER:
            printf("Ident: %s (line %d)\n", node->name, node->line);
            break;
        case AST_NUMBER:
            printf("Number: %d (line %d)\n", node->value, node->line);
            break;
        default:
            printf("Unknown AST node\n");
    }
}

void free_ast(AST *node) {
    if (!node) return;
    if (node->name) free(node->name);
    if (node->left) free_ast(node->left);
    if (node->right) free_ast(node->right);
    if (node->stmts) {
        for (int i = 0; i < node->stmt_count; ++i) free_ast(node->stmts[i]);
        free(node->stmts);
    }
    free(node);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.sl>\n", argv[0]);
        return 1;
    }

    source_file = fopen(argv[1], "r");
    if (!source_file) {
        perror("Failed to open source file");
        return 1;
    }

    lexer_init(source_file);
    advance(); 

    AST *prog = parse_program();

    printf("=== AST ===\n");
    print_ast(prog, 0);

    free_ast(prog);
    fclose(source_file);
    return 0;
}
