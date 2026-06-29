import { writeFileSync } from "fs";
import type { Program, Expr, BinaryExpr, UnaryExpr, ComparisonExpr, IdentExpr, RuleStmt } from "./ast.js";


// according to the bytecode.h

// opcode
const OP_PUSH_FACT = 0;
const OP_PUSH_CMP  = 1;
const OP_AND       = 2;
const OP_OR        = 3;
const OP_NOT       = 4;
const OP_HALT      = 5;

// comparison op
const CMP_LT = 0;
const CMP_LE = 1;
const CMP_GT = 2;
const CMP_GE = 3;
const CMP_EQ = 4;
const CMP_NE = 5;

const cmpMap: Record<string, number> = {
    "<" : CMP_LT, "<=" : CMP_LE,
    ">" : CMP_GT, ">=" : CMP_GE,
    "==": CMP_EQ, "!=" : CMP_NE,
};

function encodeInstr(op: number, cmp: number, factName: string, val: number): Buffer {
    const nameBuf = Buffer.from(factName, "utf8");
    const total = 1 + 1 + nameBuf.length + 1 + 8;
    const buf = Buffer.alloc(total);
    let off   = 0;
    buf.writeUInt8(op, off++);
    buf.writeUInt8(cmp, off++);
    nameBuf.copy(buf, off);
    off += nameBuf.length;
    buf.writeUInt8(0, off++);
    buf.writeDoubleLE(val, off);
    return buf;
}

function encodeHalt(): Buffer {
    return Buffer.from([OP_HALT]);
}

function compileExpr(expr: Expr): Buffer[] {
    switch (expr.kind) {
        case "ident": {
            const e = expr as IdentExpr;
            return [encodeInstr(OP_PUSH_FACT, 0, e.name, 0)];
        }
        case "comparison": {
            const e   = expr as ComparisonExpr;
            const cmp = cmpMap[e.operator];
            if (cmp === undefined) throw new Error(`Unknown comparison operator: ${e.operator}`);
            return [encodeInstr(OP_PUSH_CMP, cmp, e.left, e.right)];
        }
        case "binary": {
            const e      = expr as BinaryExpr;
            const left   = compileExpr(e.left);
            const right  = compileExpr(e.right);
            const opCode = e.operator === "AND" ? OP_AND : OP_OR;
            return [...left, ...right, Buffer.from([opCode])];
        }
        case "unary": {
            const e       = expr as UnaryExpr;
            const operand = compileExpr(e.operand);
            return [...operand, Buffer.from([OP_NOT])];
        }
        default:
            throw new Error(`Cannot compile expression kind: ${(expr as any).kind}`);
    }
}

function createHeader(magic: number, version: number, instrNum: number): Buffer {
    const buf = Buffer.alloc(12);
    buf.writeUInt32LE(magic,    0);
    buf.writeUInt32LE(version,  4);
    buf.writeUInt32LE(instrNum, 8);
    return buf;
}

export default function toBytecode(fileName: string, program: Program): void {
    const chunks: Buffer[] = [];
    let instrCount = 0;

    for (const stmt of program.statements) {
        if (stmt.kind !== "rule") continue;
        const expr   = (stmt as RuleStmt).expr;
        const instrs = compileExpr(expr);
        chunks.push(...instrs);
        instrCount += instrs.length;
        chunks.push(encodeHalt());
        instrCount++;
    }
    const header     = createHeader(0x524C4542, 2, instrCount);
    const final_file = fileName + ".velabc";
    writeFileSync(final_file, Buffer.concat([header, ...chunks]));
}

