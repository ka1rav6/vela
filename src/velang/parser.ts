import {
    type Expr      , type Statement    , type Program       ,
    type BinaryExpr, type UnaryExpr    , type ComparisonExpr,
    type IdentExpr , type NumberLiteral, type BoolLiteral   ,
    type RuleStmt  , type FactStmt     , type CondStmt      ,
} from "./ast.js";
import { processFile, processLine, type Token, TokenType } from "./lexer.js";

function isComparisonOp(type: TokenType): boolean {
    switch (type) {
        case TokenType.TOK_LT:
        case TokenType.TOK_GT:
        case TokenType.TOK_LE:
        case TokenType.TOK_GE:
        case TokenType.TOK_EQ:
        case TokenType.TOK_NE:
            return true;
        default:
            return false;
    }
}

export class ParseError extends Error {
    constructor(message: string) {
        super(message);
        this.name = "ParseError";
    }
}

export class Parser {
    private tokens: Token[] = [];
    private pos   : number  = 0 ;

    parse(sourceCode: string): Program {
        const lines = sourceCode.split("\n");
        const tokenLines: Token[][] = [];
        let lineNum = 0;
        for (const line of lines) {
            lineNum++;
            tokenLines.push(processLine(line, lineNum));
        }
        tokenLines.push([{ type: TokenType.TOK_EOF, text: "", number: 0 }]);
        return this.parseTokenLines(tokenLines);
    }

    parseFile(filename: string): Program {
        return this.parseTokenLines(processFile(filename));
    }

    parseTokenLines(tokenLines: Token[][]): Program {
        const statements: Statement[] = [];
        for (const line of tokenLines) {
            const stmt = this.parseLine(line);
            if (stmt !== null) {
                statements.push(stmt);
            }
        }
        return { kind: "program", statements };
    }

    private parseLine(tokens: Token[]): Statement | null {
        this.tokens = tokens;
        this.pos = 0;
        if (this.tokens.length === 0) return null;

        const first = this.peek();
        if (first.type === TokenType.TOK_EOF) return null;

        switch (first.type) {
            case TokenType.TOK_RULE:
                return this.parseRule();
            case TokenType.TOK_FACT:
                return this.parseFact();
            case TokenType.TOK_COND:
                return this.parseCond();
            default:
                throw new ParseError(`Expected statement keyword (RULE, FACT, COND) but got "${first.text}"`);
        }
    }

    private parseRule(): RuleStmt {
        this.advance();
        this.expect(TokenType.TOK_DOLLAR, "Expected '$' after RULE");
        const nameTok = this.expect(TokenType.TOK_IDENT, "Expected rule name after '$'");
        const expr    = this.parseExpression();
        return { kind: "rule", name: nameTok.text, expr };
    }

    private parseFact(): FactStmt {
        this.advance();
        this.expect(TokenType.TOK_DOLLAR, "Expected '$' after FACT");
        const nameTok = this.expect(TokenType.TOK_IDENT, "Expected fact name after '$'");

        if (this.pos >= this.tokens.length) {
            throw new ParseError("Expected fact value (number or boolean)");
        }

        const valTok = this.advance();
        if (valTok.type === TokenType.TOK_NUMBER) {
            return { kind: "fact", name: nameTok.text, value: valTok.number };
        }
        if (valTok.type === TokenType.TOK_IDENT) {
            if (valTok.text === "true") {
                return { kind: "fact", name: nameTok.text, value: true };
            }
            if (valTok.text === "false") {
                return { kind: "fact", name: nameTok.text, value: false };
            }
            throw new ParseError(`Expected number, true, or false as fact value but got "${valTok.text}"`);
        }
        throw new ParseError(`Expected number or boolean as fact value`);
    }

    private parseCond(): CondStmt {
        this.advance();
        const expr = this.parseExpression();
        return { kind: "cond", expr };
    }

    private parseExpression(): Expr {
        let left = this.parseTerm();
        while (this.pos < this.tokens.length) {
            const op = this.peek();
            if (op.type !== TokenType.TOK_AND && op.type !== TokenType.TOK_OR) break;
            this.advance();
            const right = this.parseTerm();
            left = { kind: "binary", left, right, operator: op.text as "AND" | "OR" } satisfies BinaryExpr;
        }
        return left;
    }

    private parseTerm(): Expr {
        if (this.pos < this.tokens.length && this.peek().type === TokenType.TOK_NOT) {
            this.advance();
            const operand = this.parseTerm();
            return { kind: "unary", operator: "NOT", operand } satisfies UnaryExpr;
        }
        return this.parsePrimary();
    }

    private parsePrimary(): Expr {
        if (this.pos >= this.tokens.length) {
            throw new ParseError("Unexpected end of expression");
        }

        const token = this.peek();

        if (token.type === TokenType.TOK_IDENT) {
            if (token.text === "true") {
                this.advance();
                return { kind: "bool", value: true } satisfies BoolLiteral;
            }
            if (token.text === "false") {
                this.advance();
                return { kind: "bool", value: false } satisfies BoolLiteral;
            }
            this.advance();
            if (this.pos < this.tokens.length && isComparisonOp(this.peek().type)) {
                const op = this.advance();
                const right = this.peek();
                if (right.type !== TokenType.TOK_NUMBER) {
                    throw new ParseError(`Expected number after comparison operator, got "${right.text}"`);
                }
                this.advance();
                return {
                    kind     : "comparison",
                    left     : token.text,
                    operator : op.text as ComparisonExpr["operator"],
                    right    : right.number,
                } satisfies ComparisonExpr;
            }
            return { kind: "ident", name: token.text } satisfies IdentExpr;
        }

        if (token.type === TokenType.TOK_NUMBER) {
            this.advance();
            return { kind: "number", value: token.number } satisfies NumberLiteral;
        }

        if (token.type === TokenType.TOK_LPAREN) {
            this.advance();
            const expr = this.parseExpression();
            this.expect(TokenType.TOK_RPAREN, "Expected ')' after expression");
            return expr;
        }

        throw new ParseError(`Unexpected token: "${token.text}"`);
    }

    private peek(): Token {
        return this.tokens[this.pos];
    }

    private advance(): Token {
        return this.tokens[this.pos++];
    }

    private expect(type: TokenType, message: string): Token {
        if (this.pos >= this.tokens.length) {
            throw new ParseError(message);
        }
        const token = this.peek();
        if (token.type !== type) {
            throw new ParseError(`${message} but got "${token.text}"`);
        }
        return this.advance();
    }
}
