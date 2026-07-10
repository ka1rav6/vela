#pragma once

#include "common.h"
#include <yyjson.h>
#include "rule.h"
#include "ActionEntry.h"
#include "arena.h"

typedef enum {
    VELANG,
    JSON,
}FileType;


yyjson_doc* parseJSON(const char*);

RuleEngine* build_ast(yyjson_doc*, FactDB*, EngineError*);
RuleEngine* loadBytecode(const char* file, FactDB* db, EngineError*);


