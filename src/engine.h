#include "rule.c"
#include "ConditionTree.h"

typedef struct Engine{
    FactDB* db;
    RuleEngine *r_engine;
    const char* json_file;
}Engine;

Engine* createMainEngine(FactDB*, RuleEngine*, const char*);
void destroyMainEngine(Engine*);

void linkToRule(Engine*, const char* name, Action_f, void* ); // name = rule name

void runMainEngine(Engine*);
