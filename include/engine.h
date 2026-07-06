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
EngineError registerTheAction(Engine*, const char* name, Action_f, void*);
EngineError runEngine(Engine*);
EngineError engine_get_last_error(const Engine*);
const char* engine_strerror(EngineError);

FactDB*     engine_get_factdb(Engine*);
RuleEngine* engine_get_rule_engine(Engine*);
