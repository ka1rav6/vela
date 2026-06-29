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
fact        ->"FACT" "$" IDENT val
cond        ->"COND" expression

example:
FACT $isadmin true
FACT $age     18
COND isAdmin AND (age > 18)
RULE $canAccess isAdmin
RULE $cantAccess age < 18




expression  -> term ( (AND | OR) term )*
term        -> NOT term | primary
primary     -> IDENT | '(' expression ')' | comparison

comparison  ->IDENT OP NUMBER
OP          -> < | > | <= | >= | == | !=
 *
 *
 * */

export enum TokenType{
    TOK_RULE,
    TOK_FACT,
    TOK_COND,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_DOLLAR,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_LT, TOK_GT,
    TOK_LE, TOK_GE,
    TOK_EQ, TOK_NE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_END
}

export class Token{
    type: TokenType;
    text: string = "";
    number: number = 0;

    constructor(type: TokenType, text: string);
    constructor(type: TokenType, number: number);
    constructor(type: TokenType, arg: string | number){
        this.type = type;
        if (typeof arg === "string"){
            this.text = arg;
        } else {
            this.number = arg;
        }
    }
}

export class Line{
    private tokens: Token[] = [];
    push(t: Token): void{
        this.tokens.push(t);
    }
    get length(): number{
        return this.tokens.length;
    }
    getTokens(): Token[]{
        return this.tokens;
    }
}

export class TokenStream{
    private lines: Line[] = [];
    push(l: Line): void{
        this.lines.push(l);
    }
    get length(): number{
        return this.lines.length;
    }
    getLines(): Line[]{
        return this.lines;
    }
}

function isnum(c: string): boolean{
    return c >= '0' && c <= '9';
}
function isalpha(c: string): boolean{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
function isalnum(c: string): boolean{
    return isalpha(c) || isnum(c);
}

function processLine(line: string, lineNum: number): Line{
    const l      = new Line();
    for (let i   = 0; i < line.length; ++i){
        const ch = line[i];
        switch (ch){
            case ' ': continue;
            case '#': return l;
            case '>':
                if (i + 1 < line.length && line[i + 1] === '='){
                    l.push(new Token(TokenType.TOK_GE, ">="));
                    ++i;
                } else l.push(new Token(TokenType.TOK_GT, ">"));
                break;
            case '<':
                if (i + 1 < line.length && line[i + 1] === '='){
                    l.push(new Token(TokenType.TOK_LE, "<="));
                    ++i;
                } else l.push(new Token(TokenType.TOK_LT, "<"));
                break;
            case '=':
                if (i + 1 < line.length && line[i + 1] === '='){
                    l.push(new Token(TokenType.TOK_EQ, "=="));
                    ++i;
                }
                break;
            case '!':
                if (i + 1 < line.length && line[i + 1] === '='){
                    l.push(new Token(TokenType.TOK_NE, "!="));
                    ++i;
                }
                break;
            case '$':{
                l.push(new Token(TokenType.TOK_DOLLAR, "$"));
                let ident = "";
                ++i;
                while (i < line.length && line[i] !== ' '){
                    const c = line[i];
                    if (isalnum(c) || c === '_') ident += c;
                    else {
                        throw new Error(
                            "The identifier can only contain characters from \"A-Z, a-z, 0-9, _\"\n" +
                            "The line " + lineNum + " contains an invalid identifier\n"
                        );
                    }
                    ++i;
                }
                --i;
                l.push(new Token(TokenType.TOK_IDENT, ident));
                break;
            }
            case '(':
                l.push(new Token(TokenType.TOK_LPAREN, "("));
                break;
            case ')':
                l.push(new Token(TokenType.TOK_RPAREN, ")"));
                break;
            default:
                if (isnum(ch)){
                    let num = "";
                    num += ch;
                    ++i;
                    while (i < line.length && line[i] !== ' '){
                        if (!isnum(line[i]) && line[i] !== '.'){
                            throw new Error("Number contains letter inside: line : " + lineNum);
                        }
                        num += line[i++];
                    }
                    --i;
                    l.push(new Token(TokenType.TOK_NUMBER, parseFloat(num)));
                } else if (isalpha(ch) || ch === '_'){
                    let word = "";
                    word += ch;
                    ++i;
                    while (i < line.length && (isalnum(line[i]) || line[i] === '_'))
                        word += line[i++];
                    --i;
                    if      (word === "RULE") l.push(new Token(TokenType.TOK_RULE, word));
                    else if (word === "FACT") l.push(new Token(TokenType.TOK_FACT, word));
                    else if (word === "COND") l.push(new Token(TokenType.TOK_COND, word));
                    else if (word === "AND")  l.push(new Token(TokenType.TOK_AND, word));
                    else if (word === "OR")   l.push(new Token(TokenType.TOK_OR, word));
                    else if (word === "NOT")  l.push(new Token(TokenType.TOK_NOT, word));
                    else l.push(new Token(TokenType.TOK_IDENT, word));
                }
                break;
        }
    }
    return l;
}

import fs from 'fs';

export function processFile(filename: string): TokenStream{
    const data  = fs.readFileSync(filename, 'utf8');
    const ts    = new TokenStream();
    const lines = data.split('\n');
    let lineNum = 0;
    for (const line of lines){
        lineNum++;
        ts.push(processLine(line, lineNum));
    }
    return ts;
}
