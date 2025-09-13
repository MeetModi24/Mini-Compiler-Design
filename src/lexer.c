#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static int current_line = 1;

void lexer_init(FILE *f) {
    (void)f; 
    current_line = 1;
}

void getNextToken(FILE *file, Token *token) {
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            current_line++;
            continue;
        }
        if (isspace(c)) continue;

        if (isalpha(c)) {
            int len = 0;
            token->text[len++] = c;
            while (isalnum(c = fgetc(file))) {
                if (c == '\n') { current_line++; break; }
                if (len < MAX_TOKEN_LEN - 1) token->text[len++] = c;
            }
            if (c != EOF && !isalnum(c)) ungetc(c, file);
            token->text[len] = '\0';

            if (strcmp(token->text, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(token->text, "if") == 0) token->type = TOKEN_IF;
            else token->type = TOKEN_IDENTIFIER;

            token->line = current_line;
            return;
        }

        if (isdigit(c)) {
            int len = 0;
            token->text[len++] = c;
            while (isdigit(c = fgetc(file))) {
                if (c == '\n') { current_line++; break; }
                if (len < MAX_TOKEN_LEN - 1) token->text[len++] = c;
            }
            if (c != EOF && !isdigit(c)) ungetc(c, file);
            token->text[len] = '\0';
            token->type = TOKEN_NUMBER;
            token->line = current_line;
            return;
        }

        switch (c) {
            case '=':
                {
                    int n = fgetc(file);
                    if (n == '=') {
                        token->type = TOKEN_EQUAL;
                        strcpy(token->text, "==");
                    } else {
                        if (n != EOF) ungetc(n, file);
                        token->type = TOKEN_ASSIGN;
                        strcpy(token->text, "=");
                    }
                    token->line = current_line;
                    return;
                }
            case '+':
                token->type = TOKEN_PLUS; strcpy(token->text, "+"); token->line = current_line; return;
            case '-':
                token->type = TOKEN_MINUS; strcpy(token->text, "-"); token->line = current_line; return;
            case '(':
                token->type = TOKEN_LPAREN; strcpy(token->text, "("); token->line = current_line; return;
            case ')':
                token->type = TOKEN_RPAREN; strcpy(token->text, ")"); token->line = current_line; return;
            case '{':
                token->type = TOKEN_LBRACE; strcpy(token->text, "{"); token->line = current_line; return;
            case '}':
                token->type = TOKEN_RBRACE; strcpy(token->text, "}"); token->line = current_line; return;
            case ';':
                token->type = TOKEN_SEMICOLON; strcpy(token->text, ";"); token->line = current_line; return;
            case '/':
                {
                    int n = fgetc(file);
                    if (n == '/') {
                        while ((n = fgetc(file)) != EOF && n != '\n');
                        current_line++;
                        continue;
                    } else {
                        if (n != EOF) ungetc(n, file);
                        token->type = TOKEN_UNKNOWN;
                        token->text[0] = '/';
                        token->text[1] = '\0';
                        token->line = current_line;
                        return;
                    }
                }
            default:
                token->type = TOKEN_UNKNOWN;
                token->text[0] = c;
                token->text[1] = '\0';
                token->line = current_line;
                return;
        }
    }

    token->type = TOKEN_EOF;
    strcpy(token->text, "EOF");
    token->line = current_line;
}
