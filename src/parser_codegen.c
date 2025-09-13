#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "lexer.h"

/* ---------------------------
   AST definitions
   --------------------------- */
typedef enum {
    AST_PROGRAM,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_BINOP,      
    AST_IDENTIFIER,
    AST_NUMBER,
    AST_CONDITION    
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

/* ---------------------------
   Parser state
   --------------------------- */
static Token current_token;
static FILE *source_file;

void advance();
void syntax_error(const char *fmt, ...);
AST *parse_program();
AST *parse_statement();
AST *parse_declaration();
AST *parse_assignment();
AST *parse_conditional();
AST *parse_expression();
AST *parse_term();
AST *parse_condition();
AST *make_node(ASTNodeType type, int line);
void free_ast(AST *n);
void print_ast(AST *n, int indent);

/* ---------------------------
   Simple symbol table
   --------------------------- */
#define VAR_BASE_ADDR 0x10
#define TEMP_ADDR 0x00

typedef struct {
    char *name;
    int addr;
} Symbol;

typedef struct {
    Symbol *items;
    int count;
    int cap;
} SymTable;

void sym_init(SymTable *t) { t->items = NULL; t->count = 0; t->cap = 0; }
int sym_find(SymTable *t, const char *name) {
    for (int i = 0; i < t->count; ++i) if (strcmp(t->items[i].name, name) == 0) return t->items[i].addr;
    return -1;
}
int sym_declare(SymTable *t, const char *name) {
    if (sym_find(t, name) != -1) {
        fprintf(stderr, "Semantic error: variable '%s' already declared\n", name);
        exit(EXIT_FAILURE);
    }
    if (t->count == t->cap) {
        t->cap = (t->cap == 0) ? 8 : t->cap * 2;
        t->items = realloc(t->items, sizeof(Symbol) * t->cap);
    }
    int addr = VAR_BASE_ADDR + t->count;
    t->items[t->count].name = strdup(name);
    t->items[t->count].addr = addr;
    t->count++;
    return addr;
}
void sym_free(SymTable *t) {
    for (int i = 0; i < t->count; ++i) free(t->items[i].name);
    free(t->items);
}

/* ---------------------------
   Label generator & emitter
   --------------------------- */
static int label_counter = 0;
char *new_label(const char *prefix) {
    char *buf = malloc(32);
    snprintf(buf, 32, "%s%d", prefix, label_counter++);
    return buf;
}

void emit(FILE *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    fprintf(out, "\n");
    va_end(ap);
}

/* ---------------------------
   Parser implementation
   --------------------------- */

void syntax_error(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "Syntax error (line %d): ", current_token.line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ". Got token '%s' (type %d)\n", current_token.text, current_token.type);
    exit(EXIT_FAILURE);
}

void advance() {
    getNextToken(source_file, &current_token);
}

void expect(TokenType t, const char *errmsg) {
    if (current_token.type != t) syntax_error("%s", errmsg);
    advance();
}

AST *make_node(ASTNodeType type, int line) {
    AST *n = (AST*)malloc(sizeof(AST));
    memset(n, 0, sizeof(AST));
    n->type = type;
    n->line = line;
    return n;
}

AST *parse_program() {
    AST *prog = make_node(AST_PROGRAM, current_token.line);
    AST **stmts = NULL;
    int count = 0;
    while (current_token.type != TOKEN_EOF) {
        AST *s = parse_statement();
        if (!s) break;
        stmts = realloc(stmts, sizeof(AST*) * (count + 1));
        stmts[count++] = s;
    }
    prog->stmts = stmts;
    prog->stmt_count = count;
    return prog;
}

AST *parse_statement() {
    if (current_token.type == TOKEN_INT) return parse_declaration();
    if (current_token.type == TOKEN_IDENTIFIER) return parse_assignment();
    if (current_token.type == TOKEN_IF) return parse_conditional();
    syntax_error("Expected a statement (declaration, assignment, or if)");
    return NULL;
}

AST *parse_declaration() {
    int ln = current_token.line;
    expect(TOKEN_INT, "Expected 'int' for declaration");
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier after 'int'");
    AST *n = make_node(AST_DECLARATION, ln);
    n->name = strdup(current_token.text);
    advance();
    expect(TOKEN_SEMICOLON, "Expected ';' after declaration");
    return n;
}

AST *parse_assignment() {
    int ln = current_token.line;
    if (current_token.type != TOKEN_IDENTIFIER) syntax_error("Expected identifier at assignment start");
    AST *n = make_node(AST_ASSIGNMENT, ln);
    n->name = strdup(current_token.text);
    advance();
    expect(TOKEN_ASSIGN, "Expected '=' in assignment");
    n->left = parse_expression();
    expect(TOKEN_SEMICOLON, "Expected ';' after assignment");
    return n;
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
        stmts = realloc(stmts, sizeof(AST*) * (count + 1));
        stmts[count++] = s;
    }
    expect(TOKEN_RBRACE, "Expected '}' to end if block");
    AST *n = make_node(AST_IF, ln);
    n->left = cond;
    n->stmts = stmts;
    n->stmt_count = count;
    return n;
}

AST *parse_condition() {
    int ln = current_token.line;
    AST *left = parse_expression();
    expect(TOKEN_EQUAL, "Expected '==' in condition");
    AST *right = parse_expression();
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

/* ---------------------------
   AST utilities
   --------------------------- */
void print_indent(int i) { for (int k = 0; k < i; ++k) putchar(' '); }

void print_ast(AST *node, int indent) {
    if (!node) return;
    print_indent(indent);
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (stmts=%d)\n", node->stmt_count);
            for (int i = 0; i < node->stmt_count; ++i) print_ast(node->stmts[i], indent + 2);
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
            print_indent(indent + 2); printf("Block (stmts=%d):\n", node->stmt_count);
            for (int i = 0; i < node->stmt_count; ++i) print_ast(node->stmts[i], indent + 4);
            break;
        case AST_CONDITION:
            printf("Condition:\n");
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

void free_ast(AST *n) {
    if (!n) return;
    if (n->name) free(n->name);
    if (n->left) free_ast(n->left);
    if (n->right) free_ast(n->right);
    if (n->stmts) {
        for (int i = 0; i < n->stmt_count; ++i) free_ast(n->stmts[i]);
        free(n->stmts);
    }
    free(n);
}

/* ---------------------------
   Code generation
   --------------------------- */

void codegen_expression(AST *expr, FILE *out, SymTable *sym);

void codegen_expr_store_temp(AST *expr, FILE *out, SymTable *sym) {
    codegen_expression(expr, out, sym);
    emit(out, "mov M A 0x%02X", TEMP_ADDR);          
}

void codegen_expression(AST *expr, FILE *out, SymTable *sym) {
    if (!expr) return;
    switch (expr->type) {
        case AST_NUMBER:
            emit(out, "ldi A %d", expr->value);
            return;
        case AST_IDENTIFIER: {
            int addr = sym_find(sym, expr->name);
            if (addr == -1) {
                fprintf(stderr, "Semantic error: variable '%s' used before declaration (line %d)\n", expr->name, expr->line);
                exit(EXIT_FAILURE);
            }
            emit(out, "mov A M 0x%02X", addr);
            return;
        }
        case AST_BINOP: {
            codegen_expr_store_temp(expr->left, out, sym);
            codegen_expression(expr->right, out, sym);
            emit(out, "mov B A");
            emit(out, "mov A M 0x%02X", TEMP_ADDR);
            if (expr->op == '+') emit(out, "add");
            else if (expr->op == '-') emit(out, "sub");
            else { fprintf(stderr, "Codegen error: unknown op '%c'\n", expr->op); exit(EXIT_FAILURE); }
            return;
        }
        default:
            fprintf(stderr, "Codegen error: unexpected node in expression (type %d)\n", expr->type);
            exit(EXIT_FAILURE);
    }
}

void codegen_condition(AST *cond, FILE *out, SymTable *sym) {
    if (!cond || cond->type != AST_CONDITION) {
        fprintf(stderr, "Codegen internal error: expected condition node\n");
        exit(EXIT_FAILURE);
    }
    codegen_expr_store_temp(cond->left, out, sym);
    codegen_expression(cond->right, out, sym);    
    emit(out, "mov B A");
    emit(out, "mov A M 0x%02X", TEMP_ADDR);
    emit(out, "cmp");
}

void codegen_statement(AST *stmt, FILE *out, SymTable *sym) {
    if (!stmt) return;
    switch (stmt->type) {
        case AST_DECLARATION: {
            int addr = sym_declare(sym, stmt->name);
            emit(out, "// decl %s -> 0x%02X", stmt->name, addr);
            emit(out, "ldi A 0");
            emit(out, "mov M A 0x%02X", addr);
            return;
        }
        case AST_ASSIGNMENT: {
            int addr = sym_find(sym, stmt->name);
            if (addr == -1) {
                fprintf(stderr, "Semantic error: assignment to undeclared variable '%s' (line %d)\n", stmt->name, stmt->line);
                exit(EXIT_FAILURE);
            }
            codegen_expression(stmt->left, out, sym);
            emit(out, "mov M A 0x%02X", addr);
            emit(out, "// %s := [stored at 0x%02X]", stmt->name, addr);
            return;
        }
        case AST_IF: {
            char *lab_then = new_label("L_then_");
            char *lab_end  = new_label("L_end_");
            codegen_condition(stmt->left, out, sym);
            emit(out, "jz %s", lab_then);
            emit(out, "jmp %s", lab_end);
            emit(out, "%s:", lab_then);
            for (int i = 0; i < stmt->stmt_count; ++i) codegen_statement(stmt->stmts[i], out, sym);
            emit(out, "%s:", lab_end);
            free(lab_then);
            free(lab_end);
            return;
        }
        default:
            fprintf(stderr, "Codegen error: unsupported statement type %d\n", stmt->type);
            exit(EXIT_FAILURE);
    }
}

void codegen_program(AST *prog, FILE *out, SymTable *sym) {
    emit(out, "// SimpleLang -> assembly");
    emit(out, "// TEMP at 0x%02X, variables from 0x%02X upward", TEMP_ADDR, VAR_BASE_ADDR);
    emit(out, "");
    for (int i = 0; i < prog->stmt_count; ++i) {
        codegen_statement(prog->stmts[i], out, sym);
    }
    emit(out, "hlt");
}

/* ---------------------------
   Main
   --------------------------- */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.sl> [out.asm]\n", argv[0]);
        return 1;
    }

    source_file = fopen(argv[1], "r");
    if (!source_file) { perror("Failed to open source file"); return 1; }

    lexer_init(source_file);
    advance();

    AST *prog = parse_program();

    printf("=== Parsed AST ===\n");
    print_ast(prog, 0);

    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) { perror("Failed to open output file"); free_ast(prog); fclose(source_file); return 1; }
    } else {
        printf("\n=== Generated assembly (stdout) ===\n");
    }

    SymTable sym; sym_init(&sym);
    label_counter = 0;
    codegen_program(prog, out, &sym);

    if (out != stdout) fclose(out);
    sym_free(&sym);
    free_ast(prog);
    fclose(source_file);
    return 0;
}
