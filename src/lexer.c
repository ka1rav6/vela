#include "lexer.h"


Lexer* initLexer(const char* input){
    Lexer* temp = (Lexer*)malloc(sizeof(Lexer));
    memset(temp, 0, sizeof(*temp));
    temp->input = input;
    temp->inputLen = strlen(input);
    temp->pos = 0;
    temp->readPos = 0;
    return temp;
}

void readChar(Lexer* l){
    if (l->readPos >= l->inputLen)
        l->ch = '\0';
    else
        l->ch = l->input[l->readPos];
    l->pos = l->readPos;
    l->readPos ++;
}

TokenType* nextToken(Lexer* l){
    TokenType t;
    switch (l->ch){
        case '$':
            t = TOK_DOLLAR;
        case EOF:
            t = TOK_END;
        // ALL OTHER CASES REQUIRE MORE THAN ONE LETTER 




    }
    return t;
}

