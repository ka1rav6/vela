export type Expr = BinaryExpr | UnaryExpr | ComparisonExpr | IdentExpr | NumberLiteral | BoolLiteral;

export interface BinaryExpr {
    kind: "binary";
    left: Expr;
    right: Expr;
    operator: "AND" | "OR";
}

export interface UnaryExpr {
    kind: "unary";
    operator: "NOT";
    operand: Expr;
}

export interface ComparisonExpr {
    kind: "comparison";
    left: string;
    operator: "<" | ">" | "<=" | ">=" | "==" | "!=";
    right: number;
}

export interface IdentExpr {
    kind: "ident";
    name: string;
}

export interface NumberLiteral {
    kind: "number";
    value: number;
}

export interface BoolLiteral {
    kind: "bool";
    value: boolean;
}

export type Statement = RuleStmt | FactStmt | CondStmt;

export interface RuleStmt {
    kind: "rule";
    name: string;
    action: string;
    expr: Expr;
}

export interface FactStmt {
    kind: "fact";
    name: string;
    value: number | boolean;
}

export interface CondStmt {
    kind: "cond";
    expr: Expr;
}

export interface Program {
    kind: "program";
    statements: Statement[];
}
