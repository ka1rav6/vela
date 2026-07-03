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

// a simple map that maps the operator symbol to their respective enum number
const cmpMap: Record<string, number> = {
    "<" : CMP_LT, "<=" : CMP_LE,
    ">" : CMP_GT, ">=" : CMP_GE,
    "==": CMP_EQ, "!=" : CMP_NE,
};

// encodes a string to binary and returns the binary buffer that has been created
function encodeInstr(op: number, cmp: number, factName: string, val: number): Buffer {
    const nameBuf = Buffer.from(factName, "utf8");
    const total   = 1 + 1 + nameBuf.length + 1 + 8;    // op + cmp + name + "\0" + double
    const buf     = Buffer.alloc(total);               // creates a buffer full of zeros
    let off       = 0;                                 // the main offset (ptr to the byte in the buffer)
    buf.writeUInt8(op,  off++);
    buf.writeUInt8(cmp, off++);
    nameBuf.copy(buf, off);                            // copying the factname to the buffer
    off += nameBuf.length;
    buf.writeUInt8(0, off++);                          // adding the null terminator
    buf.writeDoubleLE(val, off);                       // LE = Little Endian
    return buf;
}

function encodeHalt(): Buffer {
    return Buffer.from([OP_HALT]);    // a buffer that simply just contains the encoding of OP_HALT
}

// the main recursive function that returns a Buffer[] containing 
// the required binary bytecode
function compileExpr(expr: Expr): Buffer[] {
    switch (expr.kind) {
        case "ident": { // ident = some-name [identifier]
            const e = expr as IdentExpr;
            return [encodeInstr(OP_PUSH_FACT, 0, e.name, 0)];                   // 0 and 0 just act as padding here
        }
        case "comparison": {
            const e   = expr as ComparisonExpr
            const cmp = cmpMap[e.operator];
            if (cmp === undefined)                                              // some unknown symbol
                throw new Error(`Unknown comparison operator: ${e.operator}`);
            return [encodeInstr(OP_PUSH_CMP, cmp, e.left, e.right)];
        }
        case "binary": {
            const e      = expr as BinaryExpr;
            const left   = compileExpr(e.left);                                 // again similar recursive walking to write and produce the bytecode
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

// to create the header of each binary file
function createHeader(magic: number, version: number, instrNum: number, ruleCount: number): Buffer {
    const buf = Buffer.alloc(16);
    buf.writeUInt32LE(magic,      0);
    buf.writeUInt32LE(version,    4);
    buf.writeUInt32LE(instrNum,   8);
    buf.writeUInt32LE(ruleCount, 12);
    return buf;
}

// iterates through program statements and converts everything into buffers
// each buffer contains binary bytecode
// headers for binary files are also created here
// all buffers are compiled and concatinated in the end
export default function toBytecode(fileName: string, program: Program): void {
    const chunks: Buffer[] = [];
    let instrCount = 0;
    let ruleCount = 0;

    for (const stmt of program.statements) {
        if (stmt.kind !== "rule") continue;
        const s = stmt as RuleStmt;

        // Encode rule name: 1 byte length + name bytes (no null terminator, max 63)
        const nameBuf = Buffer.from(s.name, "utf8");
        const nameLen = Math.min(nameBuf.length, 63);
        // Encode action name: 1 byte length + action bytes
        const actionBuf = Buffer.from(s.action, "utf8");
        const actionLen = Math.min(actionBuf.length, 63);
        const meta = Buffer.alloc(1 + nameLen + 1 + actionLen);
        let off = 0;
        meta.writeUInt8(nameLen, off++);
        nameBuf.copy(meta, off, 0, nameLen);
        off += nameLen;
        meta.writeUInt8(actionLen, off++);
        actionBuf.copy(meta, off, 0, actionLen);
        off += actionLen;
        chunks.push(meta);

        // Compile condition expression
        const instrs = compileExpr(s.expr);
        chunks.push(...instrs);
        instrCount += instrs.length;
        chunks.push(encodeHalt());
        instrCount++;
        ruleCount++;
    }
    const header     = createHeader(0x524C4542, 3, instrCount, ruleCount);
    const final_file = fileName + ".velabc";
    writeFileSync(final_file, Buffer.concat([header, ...chunks]));
}

