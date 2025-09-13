#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKEN_LEN 100

typedef enum {
    TOKEN_INT,
    TOKEN_IF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char text[MAX_TOKEN_LEN];
} Token;

void getNextToken(FILE *file, Token *token){
    int c;
    while ((c = fgetc(file)) != EOF){
        if (isspace(c)) continue;

        if (isalpha(c)) {
            int len = 0;
            token->text[len++] = c;
            while (isalnum(c = fgetc(file))) {
                if (len < MAX_TOKEN_LEN - 1) token->text[len++] = c;
            }
            ungetc(c, file);
            token->text[len] = '\0';

            if (strcmp(token->text, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(token->text, "if") == 0) token->type = TOKEN_IF;
            else token->type = TOKEN_IDENTIFIER;
            return;
        }

        if (isdigit(c)) {
            int len = 0;
            token->text[len++] = c;
            while (isdigit(c = fgetc(file))) {
                if (len < MAX_TOKEN_LEN - 1) token->text[len++] = c;
            }
            ungetc(c, file);
            token->text[len] = '\0';
            token->type = TOKEN_NUMBER;
            return;
        }

        switch (c) {
            case '=':
                if ((c = fgetc(file)) == '=') {
                    token->type = TOKEN_EQUAL;
                    strcpy(token->text, "==");
                } else {
                    ungetc(c, file);
                    token->type = TOKEN_ASSIGN;
                    strcpy(token->text, "=");
                }
                return;
            case '+':
                token->type = TOKEN_PLUS; strcpy(token->text, "+"); return;
            case '-':
                token->type = TOKEN_MINUS; strcpy(token->text, "-"); return;
            case '(':
                token->type = TOKEN_LPAREN; strcpy(token->text, "("); return;
            case ')':
                token->type = TOKEN_RPAREN; strcpy(token->text, ")"); return;
            case '{':
                token->type = TOKEN_LBRACE; strcpy(token->text, "{"); return;
            case '}':
                token->type = TOKEN_RBRACE; strcpy(token->text, "}"); return;
            case ';':
                token->type = TOKEN_SEMICOLON; strcpy(token->text, ";"); return;
            default:
                token->type = TOKEN_UNKNOWN;
                token->text[0] = c;
                token->text[1] = '\0';
                return;
        }
    }
    token->type = TOKEN_EOF;
    strcpy(token->text, "EOF");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    Token token;
    do {
        getNextToken(file, &token);
        printf("Token: %d, Text: %s\n", token.type, token.text);
    } while (token.type != TOKEN_EOF);

    fclose(file);
    return 0;
}
 