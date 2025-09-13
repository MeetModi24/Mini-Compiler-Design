#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

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
    int line; 
} Token;

void lexer_init(FILE *f);

void getNextToken(FILE *file, Token *token);

#endif 
