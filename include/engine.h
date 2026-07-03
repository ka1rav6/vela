#pragma once
#include "rule.h"
#include "ConditionTree.h"
#include "parser_engine.h"
#include "ActionEntry.h"
#include "arena.h"

/* Opaque handle. Internals are hidden in engine_internal.h. */
typedef struct Engine Engine;

Engine* createEngine(const char*, FileType);
void deleteEngine(Engine*);
int registerTheAction(Engine*, const char* name, Action_f, void*);
int runEngine(Engine*);

// Read-only accessors
FactDB*     engine_get_factdb(Engine*);
RuleEngine* engine_get_rule_engine(Engine*);
