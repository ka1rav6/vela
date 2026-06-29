import { TokenType as t_type} from "./lexer.js"

export interface Expr{

}

export interface BinaryExpr{
    left      : Expr                 ;
    right     : Expr                 ;
    operation ?: "AND" | "NOT" | "OR";
}

export interface RuleExpr{
    decl : "RULE" | t_type.TOK_RULE ;
    name : t_type.TOK_IDENT         ;
    expr : BinaryExpr               ;
}

export interface FactExpr{
    decl  : "FACT" | t_type.TOK_FACT;
    name  : t_type.TOK_IDENT        ;
    value : number | boolean | Expr ;
}


