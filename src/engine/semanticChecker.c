#include "semanticChecker.h"
#include "rule.h"
#include "factdb_internal.h"

bool isOperator(const char* op)
{
    const char* ops[10] = {">=", "<=", "==", "!=", ">", "<", "and", "or", "not", "null"};
    for (int i = 0; i < 10; i++)
    {
        if (strcmp(op, ops[i]) == 0)
            return true;
    }
    return false;
}

bool isComparisonCorrect(FactDB* db, const char* factname)
{
    return !factdb_has_bool(db, factname);
}

bool factExists(FactDB* db, const char* fact, factType t)
{
    switch (t)
    {
        case BOOL: return factdb_has_bool(db, fact);
        case NUM:  return factdb_has_num(db, fact);
        case STR:  return factdb_has_str(db, fact);
    }
    return false;
}

bool duplicateRule(RuleEngine* e, const char* name)
{
    return findRule(e, name) != NULL;
}

bool isMixedBoolNumArray(FactDB* db, yyjson_val* arr, EngineError* err)
{
    size_t len = yyjson_arr_size(arr);
    bool hasBool = false;
    bool hasNum  = false;
    bool hasStr  = false;

    for (size_t i = 0; i < len; i++)
    {
        yyjson_val* elem = yyjson_arr_get(arr, i);
        if (!yyjson_is_str(elem))
            continue;
        const char* name = yyjson_get_str(elem);
        if (factExists(db, name, BOOL))
            hasBool = true;
        if (factExists(db, name, NUM))
            hasNum  = true;
        if (factExists(db, name, STR))
            hasStr  = true;
        if ((hasBool && hasNum) || (hasBool && hasStr) || (hasNum && hasStr))
        {
            if (err) *err = ENGINE_ERR_BOOL_COMPARED;
            return true;
        }
    }
    return false;
}

bool isEmptyOrUndersizedArray(yyjson_val* arr, const char* op)
{
    size_t len = yyjson_arr_size(arr);
    if (strcmp(op, "not") == 0 || strcmp(op, "null") == 0)
        return len < 1;
    return len < 2;
}
