#include "ConditionTree.h"


Node* createNode(Arena* ar, Type t)
{
    Node* temp = (Node*)arena_alloc(ar, sizeof(Node));
    if (!temp) return NULL;
    memset(temp, 0, sizeof(Node));
    temp->type = t;
    return temp;
}
