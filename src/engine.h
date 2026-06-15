#include "rule.c"
#include "ConditionTree.h"

typedef struct Engine{
    FactDB* db;
    RuleEngine *r;
    const char* json_file;
}Engine;

Engine* createMainEngine(const char*);
Engine* createMainEngine(FactDB*, RuleEngine*, const char*);
void destroyEngine(Engine*);

