#pragma once

#include "rule.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include "arena.h"
/*
NOTE THE ASSUMPTIONS:
1. Language has only keywords : RULE, FACT, COND, AND, OR, NOT (ALWAYS ALL CAPS)
2. Rest all names etc created by the user must be unique
3. The user should be able to update particular data points and it will not change on the file (as that is more expensive)
   and rather be changed in the tree straight away
4. Each rule/ fact or condition is written on a separate line with a simple identifier (of keywords)
5. each rule can be named something specific using this format:
                 RULE $RULE_NAME ruleconditions 
                 (note the dollar sign)
6. Single line comments can be created by using a '#' (HASHTAG)
7. normal parenthesis can be used to nest conditions
*/

/*
 * INFORMAL GRAMMAR PLAN:


program     -> statement*

statement   -> rule | fact | cond

rule        -> "RULE" "$" IDENT expression
fact        ->"FACT" "$" IDENT
cond        ->"COND" expression

expression  -> term ( (AND | OR) term )*
term        -> NOT term | primary
primary     -> IDENT | '(' expression ')' | comparison

comparison  ->IDENT OP NUMBER
OP          -> < | > | <= | >= | == | !=
 *
 *
 * */

namespace VelaLang{
enum class TokenType{
    TOK_RULE,
    TOK_FACT,
    TOK_COND,
    TOK_AND, // AND
    TOK_OR, // OR
    TOK_NOT, // NOT
    TOK_DOLLAR, // $
    TOK_IDENT, 
    TOK_NUMBER,
    TOK_LT, TOK_GT, // < >
    TOK_LE, TOK_GE, // <=  >=
    TOK_EQ, TOK_NE, // ==  !=
    TOK_LPAREN, // (
    TOK_RPAREN, // )
    TOK_END  // EOF
};

class Token{
public:
    TokenType type;
    std::string text; // for identifiers
    double number;    // for numeric values
    Token(TokenType, std::string, double);
    Token(TokenType, const std::string);
    Token(TokenType, double);
};



class Line{
private:
    std::vector<Token> tokens;
public:
    Line()  = default;
    ~Line() = default;
    Line(const Line&) = default;
    Line(Line&&) = default;
    Line& operator<<(const Token& t){
        tokens.emplace_back(t);
        return *this;
    }
    size_t getLine(){
        return tokens.size();
    }
    std::vector<Token> getTokens(){
        return tokens;
    }
};

class TokenStream{
    private:
        std::vector<Line> lines;
    public:
        TokenStream()  = default;
        ~TokenStream() = default;
        TokenStream(const TokenStream&) = default;
        TokenStream(TokenStream&&) = default;
        TokenStream& operator<<(const Line& l){
            lines.emplace_back(l);
            return *this;
        }
        size_t getLine(){
            return lines.size();
        }
        std::vector<Line> getTokens(){
            return lines;
        }
};
TokenStream* processFile(const std::string filename, Arena* ar);
} // namespace VelaLang
