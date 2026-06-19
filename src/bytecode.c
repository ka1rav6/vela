#include "../include/bytecode.h"

static Bytecode* createBytecode(Arena* ar){
    Bytecode* bc = (Bytecode*)arena_alloc(ar, sizeof(Bytecode));
    bc->capacity = 8;
    bc->count = 0;
    bc->code = (Instr*)arena_alloc(ar, sizeof(Instr) * bc->capacity);
    return bc;
}

static void emit(Arena* ar, Bytecode* bc, Instr i){
    if (bc->count >= bc->capacity){
        Instr* old = bc->code;
        int newCap = bc->capacity * 2;
        Instr* newGrown = (Instr*)arena_alloc(ar, sizeof(Instr) * newCap);
        memcpy(newGrown, old, sizeof(Instr) * bc->count);
        bc->code = newGrown;
        bc->capacity = newCap;
    }
    bc->code[bc->count++] = i;
}
