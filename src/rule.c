#include "rule.h"

void run(RuleEngine* e, FactDB* db){
    for (int i = 0; i < e->ruleCount; i ++){
        if (evaluate(db, e->rules[i].condition)){
            // TRIGGER THE ACTION
            printf("Action Triggered: %s\n", e->rules[i].action);
        }
    }
}


Rule* createRule(Node* n, char* action, char* name){
    Rule* temp = (Rule*)malloc(sizeof(Rule));
    if (temp == NULL){
        printf("COULD NOT ALLOCATE SPACE FOR RULE\n");
        exit(1);
    }
    memset(temp, 0, sizeof(Rule));
    temp->condition = n;
    temp->action = action;
    temp->ruleName = name;
    return temp;
}

RuleEngine* createEngine(){
    RuleEngine* temp = (RuleEngine*)malloc(sizeof(RuleEngine));
    if (temp == NULL){
    printf("COULD NOT ALLOCATE SPACE FOR RULE\n");
    exit(1);
    }
    memset(temp, 0, sizeof(RuleEngine));
    temp->rules = NULL;
    temp->ruleCount = 0;
    return temp;
}
void deleteRule(Rule* r){
    deleteNode(r->condition);
    free(r->action);
    r->action = NULL;
    free(r->ruleName);
    r->ruleName = NULL;
    free(r);
    r = NULL;
}

void deleteEngine(RuleEngine* RE){
    for (int i = 0; i < RE->ruleCount; i++){
        deleteRule(&RE->rules[i]);
    }
    free(RE);
    RE = NULL;
}











