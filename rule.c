#include "rule.h"

void run(RuleEngine* e, FactDB* db){
    for (int i = 0; i < e->ruleCount; i ++){
        if (evaluate(db, e->rules[i].condition)){
            // TRIGGER THE ACTION
            printf("Action Triggered: %s\n", e->rules[i].action);
        }
    }
}
