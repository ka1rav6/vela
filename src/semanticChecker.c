#include "semanticChecker.h"
#include "rule.h"
/*
 * checks to include:
 * 1. factname not in db but in expression
 * 2. unknown operator
 * 3. duplicate rule names
 * 4. and of "age" and "isAdmin" (for example)
 * 5. empty expressions : "and" : []
 * 6. missing action
 */

bool isOperator(const char* op){
    const char* ops[9] = {">=", "<=","==", "!=", ">", "<", "and", "or", "not"};
    for (int i = 0; i < 9; i++){
        if (strcmp(op, ops[i]) == 0)
            return true;
    }
    return false;
}


bool isComparisonCorrect(FactDB* db, const char* factname){
    // to check if the fact is of bool type but is used in a comparison expression
    for (int i = 0; i < db->boolCount; i++){
        if (strcmp(db->boolFacts[i].name, factname) == 0){
            return false;
        }
    }
    return true;
}

bool factExists(FactDB* db, const char* fact, factType t){
    switch (t){
        case BOOL:
            for (int i = 0 ; i < db->boolCount; i++){
                if (strcmp(db->boolFacts[i].name, fact) == 0)
                    return true;
            }
            break;
        case NUM:
            for (int i = 0 ; i < db->numCount; i++){
                if (strcmp(db->numFacts[i].name, fact) == 0)
                    return true;
            }
            break;
    }
    return false;
}

bool duplicateRule(RuleEngine* e, const char* name){
    for (int i = 0; i < e->ruleCount; i++){
        if (strcmp(e->rules[i].ruleName,name) == 0)
            return true;
    }
    return false;
}
