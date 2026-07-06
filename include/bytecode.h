#pragma once
#include "common.h"
#include "ConditionTree.h"
#include "factdb.h"
#include "arena.h"

typedef enum {
    OP_PUSH_FACT,
    OP_PUSH_CMP,
    OP_PUSH_STR_CMP,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_HALT
} OpCode;

typedef struct {
    char* factName;
    double val;
    char* strVal;
    OpCode op;
    CompareOp cmp;
} Instr;

typedef struct {
    Instr* code;
    int count;
    int capacity;
} Bytecode;

typedef enum {
    VM_FALSE = 0,
    VM_TRUE  = 1,
    VM_ERROR = -1
} VMResult;

Bytecode* compileNode(Arena*, Node*);
VMResult runBytecode(FactDB*, Bytecode*);


