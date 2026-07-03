#include "bytecode.h"

enum { STACK_MAX = 256 };

static int countInstr(Node* n)
{
    if (!n) return 0;
    switch (n->type)
    {
        case NODE_AND:
        case NODE_OR:
            return countInstr(n->data.op.left) + countInstr(n->data.op.right) + 1;
        case NODE_NOT:
            return countInstr(n->data.unary.child) + 1;
        case NODE_FACT:
        case NODE_COMPARE:
            return 1;
    }
    return 0;
}

static void compileWalk(Bytecode* bc, Node* n, int* pos)
{
    switch (n->type)
    {
        case NODE_AND:
            compileWalk(bc, n->data.op.left, pos);
            compileWalk(bc, n->data.op.right, pos);
            bc->code[(*pos)++] = (Instr){ .op = OP_AND };
            break;
        case NODE_OR:
            compileWalk(bc, n->data.op.left, pos);
            compileWalk(bc, n->data.op.right, pos);
            bc->code[(*pos)++] = (Instr){ .op = OP_OR };
            break;
        case NODE_NOT:
            compileWalk(bc, n->data.unary.child, pos);
            bc->code[(*pos)++] = (Instr){ .op = OP_NOT };
            break;
        case NODE_FACT:
            bc->code[(*pos)++] = (Instr){
                .op       = OP_PUSH_FACT,
                .factName = n->data.Fact.factName,
            };
            break;
        case NODE_COMPARE:
            bc->code[(*pos)++] = (Instr){
                .op       = OP_PUSH_CMP,
                .factName = n->data.Compare.factName,
                .cmp      = n->data.Compare.op,
                .val      = n->data.Compare.val,
            };
            break;
    }
}

Bytecode* compileNode(Arena* ar, Node* n)
{
    int total   = countInstr(n) + 1;
    Bytecode* bc = (Bytecode*)arena_alloc(ar, sizeof(Bytecode));
    bc->code     = (Instr*)arena_alloc(ar, sizeof(Instr) * total);
    bc->count    = total;
    bc->capacity = total;
    int pos = 0;
    compileWalk(bc, n, &pos);
    bc->code[pos++] = (Instr){ .op = OP_HALT };
    return bc;
}

static bool runCompare(FactDB* db, Instr* i)
{
    double lhs = getNumFact(db, i->factName);
    double rhs = i->val;
    switch (i->cmp)
    {
        case OP_LT: return lhs <  rhs;
        case OP_GT: return lhs >  rhs;
        case OP_LE: return lhs <= rhs;
        case OP_GE: return lhs >= rhs;
        case OP_EQ: return lhs == rhs;
        case OP_NE: return lhs != rhs;
    }
    return false;
}

bool runBytecode(FactDB* db, Bytecode* bc)
{
    bool stack[256];
    int sp = 0;
    for (int pc = 0; pc < bc->count; pc++)
    {
        Instr* i = &bc->code[pc];
        switch (i->op)
        {
            case OP_PUSH_FACT:
                if (sp >= STACK_MAX) return false;
                stack[sp++] = getBoolFact(db, i->factName);
                break;
            case OP_PUSH_CMP:
                if (sp >= STACK_MAX) return false;
                stack[sp++] = runCompare(db, i);
                break;
            case OP_AND:
                if (sp < 2) return false;
                stack[sp - 2] = stack[sp - 2] && stack[sp - 1];
                sp--;
                break;
            case OP_OR:
                if (sp < 2) return false;
                stack[sp - 2] = stack[sp - 2] || stack[sp - 1];
                sp--;
                break;
            case OP_NOT:
                if (sp < 1) return false;
                stack[sp - 1] = !stack[sp - 1];
                break;
            case OP_HALT:
                if (sp < 1) return false;
                return stack[sp - 1];
        }
    }
    return false;
}
