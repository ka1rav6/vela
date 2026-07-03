import { describe, it } from "node:test";
import assert from "node:assert/strict";

import { processLine, TokenType } from "./lexer.js";
import type { Token } from "./lexer.js";
import { Parser, ParseError } from "./parser.js";
import type {
    BinaryExpr, UnaryExpr, ComparisonExpr,
    IdentExpr, RuleStmt, FactStmt, CondStmt,
} from "./ast.js";

const parser = new Parser();

describe("Lexer", () => {

    it("tokenizes a RULE statement", () => {
        const toks = processLine("RULE $canAccess isAdmin", 1);
        assert.equal(toks.length, 4);
        assert.equal(toks[0].type, TokenType.TOK_RULE);
        assert.equal(toks[1].type, TokenType.TOK_DOLLAR);
        assert.equal(toks[2].type, TokenType.TOK_IDENT);
        assert.equal(toks[2].text, "canAccess");
        assert.equal(toks[3].type, TokenType.TOK_IDENT);
        assert.equal(toks[3].text, "isAdmin");
    });

    it("tokenizes a FACT statement with number", () => {
        const toks = processLine("FACT $age 18", 1);
        assert.equal(toks.length, 4);
        assert.equal(toks[0].type, TokenType.TOK_FACT);
        assert.equal(toks[1].type, TokenType.TOK_DOLLAR);
        assert.equal(toks[2].text, "age");
        assert.equal(toks[3].type, TokenType.TOK_NUMBER);
        assert.equal(toks[3].number, 18);
    });

    it("tokenizes comparison operators", () => {
        const toks = processLine("age >= 18", 1);
        assert.equal(toks.length, 3);
        assert.equal(toks[1].type, TokenType.TOK_GE);
        assert.equal(toks[1].text, ">=");
    });

    it("tokenizes parentheses and boolean keywords", () => {
        const toks = processLine("COND isAdmin AND (age > 18)", 1);
        assert.equal(toks.length, 8);
        assert.equal(toks[0].type, TokenType.TOK_COND);
        assert.equal(toks[1].text, "isAdmin");
        assert.equal(toks[2].type, TokenType.TOK_AND);
        assert.equal(toks[3].type, TokenType.TOK_LPAREN);
        assert.equal(toks[4].text, "age");
        assert.equal(toks[5].type, TokenType.TOK_GT);
        assert.equal(toks[6].type, TokenType.TOK_NUMBER);
        assert.equal(toks[6].number, 18);
        assert.equal(toks[7].type, TokenType.TOK_RPAREN);
    });

    it("skips comments starting with #", () => {
        const toks = processLine("# this is a comment", 1);
        assert.equal(toks.length, 0);
    });

    it("handles NOT operator", () => {
        const toks = processLine("NOT isAdmin", 1);
        assert.equal(toks.length, 2);
        assert.equal(toks[0].type, TokenType.TOK_NOT);
        assert.equal(toks[1].text, "isAdmin");
    });

    it("treats true/false as IDENT tokens", () => {
        const toks = processLine("FACT $flag true", 1);
        assert.equal(toks[3].type, TokenType.TOK_IDENT);
        assert.equal(toks[3].text, "true");
    });

    it("numbers work next to parentheses", () => {
        const toks = processLine("(age > 18)", 1);
        assert.equal(toks.length, 5);
        assert.equal(toks[3].type, TokenType.TOK_NUMBER);
        assert.equal(toks[3].number, 18);
        assert.equal(toks[4].type, TokenType.TOK_RPAREN);
    });
});

describe("Parser", () => {

    it("parses a RULE with simple ident expr", () => {
        const prog = parser.parse("RULE $canAccess $GRANT isAdmin");
        assert.equal(prog.kind, "program");
        assert.equal(prog.statements.length, 1);
        const stmt = prog.statements[0] as RuleStmt;
        assert.equal(stmt.kind, "rule");
        assert.equal(stmt.name, "canAccess");
        assert.equal(stmt.action, "GRANT");
        assert.equal(stmt.expr.kind, "ident");
        assert.equal((stmt.expr as IdentExpr).name, "isAdmin");
    });

    it("parses a FACT with number value", () => {
        const prog = parser.parse("FACT $age 18");
        const stmt = prog.statements[0] as FactStmt;
        assert.equal(stmt.kind, "fact");
        assert.equal(stmt.name, "age");
        assert.equal(stmt.value, 18);
    });

    it("parses a FACT with boolean value", () => {
        const prog = parser.parse("FACT $isAdmin true");
        const stmt = prog.statements[0] as FactStmt;
        assert.equal(stmt.value, true);
    });

    it("parses a COND with AND expression", () => {
        const prog = parser.parse("COND isAdmin AND age >= 18");
        const stmt = prog.statements[0] as CondStmt;
        assert.equal(stmt.kind, "cond");
        assert.equal(stmt.expr.kind, "binary");
        const bin = stmt.expr as BinaryExpr;
        assert.equal(bin.operator, "AND");
        assert.equal((bin.left as IdentExpr).name, "isAdmin");
        assert.equal((bin.right as ComparisonExpr).operator, ">=");
    });

    it("parses OR operator", () => {
        const prog = parser.parse("COND a OR b");
        const bin = (prog.statements[0] as CondStmt).expr as BinaryExpr;
        assert.equal(bin.operator, "OR");
    });

    it("parses NOT unary operator", () => {
        const prog = parser.parse("COND NOT isAdmin");
        const un = (prog.statements[0] as CondStmt).expr as UnaryExpr;
        assert.equal(un.operator, "NOT");
        assert.equal((un.operand as IdentExpr).name, "isAdmin");
    });

    it("NOT has higher precedence than AND", () => {
        const prog = parser.parse("COND NOT a AND b");
        const bin = (prog.statements[0] as CondStmt).expr as BinaryExpr;
        assert.equal(bin.operator, "AND");
        assert.equal((bin.left as UnaryExpr).operator, "NOT");
        assert.equal(((bin.left as UnaryExpr).operand as IdentExpr).name, "a");
        assert.equal((bin.right as IdentExpr).name, "b");
    });

    it("parses parenthesized expressions", () => {
        const prog = parser.parse("COND (a AND b) OR c");
        const bin = (prog.statements[0] as CondStmt).expr as BinaryExpr;
        assert.equal(bin.operator, "OR");
        assert.equal((bin.left as BinaryExpr).operator, "AND");
        assert.equal((bin.right as IdentExpr).name, "c");
    });

    it("parses comparison operators", () => {
        const testCases: [string, string][] = [
            ["a < 1", "<"], ["a > 1", ">"],
            ["a <= 1", "<="], ["a >= 1", ">="],
            ["a == 1", "=="], ["a != 1", "!="],
        ];
        for (const [input, op] of testCases) {
            const prog = parser.parse(`COND ${input}`);
            const cmp = (prog.statements[0] as CondStmt).expr as ComparisonExpr;
            assert.equal(cmp.operator, op, `Failed for ${input}`);
            assert.equal(cmp.left, "a");
            assert.equal(cmp.right, 1);
        }
    });

    it("multiple statements in one source", () => {
        const src = [
            "FACT $admin true",
            "FACT $age 25",
            "RULE $canAccess $GRANT admin AND (age >= 18)",
        ].join("\n");
        const prog = parser.parse(src);
        assert.equal(prog.statements.length, 3);
        assert.equal(prog.statements[0].kind, "fact");
        assert.equal(prog.statements[1].kind, "fact");
        assert.equal(prog.statements[2].kind, "rule");
    });

    it("skips comment lines", () => {
        const prog = parser.parse("# just a comment\nRULE $x $GRANT y");
        assert.equal(prog.statements.length, 1);
    });

    it("skips empty lines", () => {
        const prog = parser.parse("\n\nRULE $x $GRANT y\n\n");
        assert.equal(prog.statements.length, 1);
    });
});

describe("Parser Errors", () => {
    it("throws on unknown keyword", () => {
        assert.throws(() => parser.parse("BLAH $x y"), ParseError);
    });

    it("throws on RULE without dollar before name", () => {
        assert.throws(() => parser.parse("RULE x $GRANT y"), ParseError);
    });

    it("throws on RULE without dollar before action", () => {
        assert.throws(() => parser.parse("RULE $x GRANT y"), ParseError);
    });

    it("throws on FACT without value", () => {
        assert.throws(() => parser.parse("FACT $x"), ParseError);
    });

    it("throws on FACT with bad value", () => {
        assert.throws(() => parser.parse("FACT $x maybe"), ParseError);
    });

    it("throws on missing closing paren", () => {
        assert.throws(() => parser.parse("COND (a AND b"), ParseError);
    });

    it("throws on comparison with non-number", () => {
        assert.throws(() => parser.parse("COND a > b"), ParseError);
    });
});
