#include "factdb.h"

double getNumFact(FactDB* db, const char* name){
    for (int i = 0; i < db->numCount; i++){
        if (strcmp(db->numFacts[i].name, name) == 0)
            return db->numFacts[i].val;
    }
    return NOT_FOUND;
}

bool getBoolFact(FactDB* db, const char* name){
    for (int i = 0; i < db->boolCount; i++){
        if (strcmp(db->boolFacts[i].name, name) == 0)
            return db->boolFacts[i].val;
    }
    return NOT_FOUND;
}

bool evaluate(FactDB* db, Node* n){
    switch(n->type){
        case NODE_AND:
            return evaluate(db, n->data.op.left) && evaluate(db, n->data.op.right);
        case NODE_OR:
            return evaluate(db, n->data.op.left) || evaluate(db, n->data.op.right);
        case NODE_NOT:
            return !evaluate(db, n->data.unary.child);
        case NODE_FACT:
            return getBoolFact(db, n->data.Fact.factName);
        case NODE_COMPARE:{
            double lhs = getNumFact(db, n->data.Fact.factName);
            double rhs = n->data.Compare.val;
            switch(n->data.Compare.op){
                case OP_LT:
                    return lhs < rhs;
                case OP_GT:
                    return lhs > rhs;
                case OP_LE:
                    return lhs <= rhs;
                case OP_GE:
                    return lhs >= rhs;
                case OP_EQ:
                    return lhs == rhs;
                case OP_NE:
                    return lhs != rhs;
            }
        }
    }

}
