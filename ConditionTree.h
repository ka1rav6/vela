
#pragma once
#include <stdio.h>
#include <stdlib.h>

#define MAX_NAME 100


typedef enum {
    NODE_AND,
    NODE_OR,
    NODE_NOR,
    NODE_FACT,
    NODE_COMPARE
}Type;

typedef enum{
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQ,
    OP_NE
}CompareOp;

typedef struct Node{
    Type* type;
    
    union{
        struct {
            struct Node* left;
            struct Node* right;
        }op;
        struct {
            struct Node* child;
        }unary;
        
        struct {
            char factName[MAX_NAME];
        }Fact;
        
        struct{
            char factName[MAX_NAME];
            CompareOp op;
            double value;
        } Compare;

    } data;
} Node;
