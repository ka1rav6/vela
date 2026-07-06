#pragma once

#include "common.h"
#include "arena.h"

#define MAX_NAME 100

typedef enum {
    NODE_AND,
    NODE_OR,
    NODE_NOT,
    NODE_FACT,
    NODE_COMPARE,
    NODE_NULL,
    NODE_STR_CMP,
} Type;

typedef enum {
    OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE
} CompareOp;

typedef struct Node {
    union {
        struct { struct Node* left; struct Node* right; } op;
        struct { struct Node* child; } unary;
        struct { char *factName; } Fact;
        struct { char *factName; double val; CompareOp op; } Compare;
        struct { char *factName; char *strVal; CompareOp op; } StrCmp;
    } data;
    Type type;
} Node;

Node* createNode(Arena* ar, Type t);
