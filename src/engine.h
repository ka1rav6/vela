#include "rule.c"
#include "ConditionTree.h"

typedef struct Engine{
    FactDB* db;
    RuleEngine *r_engine;
    const char* json_file;
}Engine;

Engine* createMainEngine(FactDB*, RuleEngine*, const char*);
void destroyEngine(Engine*);

