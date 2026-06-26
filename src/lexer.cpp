#include "../include/lexer.h"

#include <fstream>

namespace VelaLang{
Token::Token(TokenType type, const std::string s){
    this->type = type;
    this->text = s;
}

Token::Token(TokenType type, double num){
    this->type = type;
    this->number = num;
}

static bool isnum(char c){
    return std::isdigit(static_cast<unsigned char>(c));
}


static Line* processLine(const std::string& line, Arena* ar, size_t line_num){
    Line* l = new (arena_alloc(ar, sizeof(Line))) Line();
    for (size_t i = 0; i < line.size(); ++i) {
    char ch = line[i];
    switch (ch) {
        case ' ': continue;
        case '#': return l;
        case '>':
            if (i + 1 < line.size() && line[i + 1] == '=') {
                *l << Token(TokenType::TOK_GE, ">=");
                ++i;
            } else *l << Token(TokenType::TOK_GT, ">");
            break;

        case '<':
            if (i + 1 < line.size() && line[i + 1] == '=') {
                *l << Token(TokenType::TOK_LE, "<=");
                ++i;
            } else *l << Token(TokenType::TOK_LT, "<");
            break;

        case '=':
            if (i + 1 < line.size() && line[i + 1] == '=') {
                *l << Token(TokenType::TOK_EQ, "==");
                ++i;
            }
            break;
        case '!':
            if (i + 1 < line.size() && line[i + 1] == '=') {
                *l << Token(TokenType::TOK_NE, "!=");
                ++i;
            }
            break;
        case '$':{
            *l << Token(TokenType::TOK_DOLLAR, "$");
            std::string ident;
            ++i;
            while (i < line.size() && line[i] != ' '){
                char c = line[i];
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ident += c;
                else {
                    std::string error_msg = "The identifier can only contain characters from \"A-Z, a-z, 0-9, _\"\n";
                    error_msg += "The line " + std::to_string(line_num) + " contains an invalid identifier\n";
                    throw std::runtime_error(error_msg);
                }
                ++i;
            }
            --i;
            *l << Token(TokenType::TOK_IDENT, ident);
            break;
        }
        case '(':
            *l << Token(TokenType::TOK_LPAREN, "(");
            break;
        case ')':
            *l << Token(TokenType::TOK_RPAREN, ")");
            break;
        default:
            if (isnum(ch)){
                std::string num;
                num += ch;
                ++i;
                while (i < line.size() && line[i] != ' '){
                    if (!isnum(line[i]) && line[i] != '.'){
                        throw std::runtime_error("Number contains letter inside: line : " + std::to_string(line_num));
                    }
                    num += line[i];
                    ++i;
                }
                --i;
                *l << Token(TokenType::TOK_NUMBER, std::stod(num));
            } else if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                std::string word;
                word += ch;
                ++i;
                while (i < line.size() && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) {
                    word += line[i];
                    ++i;
                }
                --i;
                if (word == "RULE") *l << Token(TokenType::TOK_RULE, word);
                else if (word == "FACT") *l << Token(TokenType::TOK_FACT, word);
                else if (word == "COND") *l << Token(TokenType::TOK_COND, word);
                else if (word == "AND") *l << Token(TokenType::TOK_AND, word);
                else if (word == "OR") *l << Token(TokenType::TOK_OR, word);
                else if (word == "NOT") *l << Token(TokenType::TOK_NOT, word);
                else *l << Token(TokenType::TOK_IDENT, word);
            }
            break;
    }
}
    return l;
}


TokenStream* processFile(const std::string filename, Arena* ar){
    std::fstream f(filename, std::ios::in);
    TokenStream* ts = new (arena_alloc(ar, sizeof(TokenStream))) TokenStream();
    std::string line;
    size_t line_num = 0;
    while(getline(f, line)){
        line_num ++;
        *ts << *processLine(line, ar, line_num);
    }
    return ts;
}
} // namespace VelaLang
