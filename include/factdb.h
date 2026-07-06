#pragma once

#include "common.h"
#include "ConditionTree.h"
#include <math.h>

#define MAX_FACTS 300
#define NOT_FOUND NAN

typedef enum {
    BOOL, NUM, STR
} factType;

/* Opaque handle. Internals (hash tables, locking) are hidden in factdb_internal.h. */
typedef struct FactDB FactDB;

double   getNumFact(FactDB*, const char*);
bool     getBoolFact(FactDB*, const char*);
char*    getStringFact(FactDB*, const char*);
FactDB*  createFactDB(void);
void     deleteFactDB(FactDB*);
void     setBoolFact(FactDB*, const char*, bool);
void     setNumFact(FactDB*, const char*, double);
void     setStringFact(FactDB*, const char*, const char*);
bool     factdb_has_fact(FactDB*, const char*, factType);

void printFactDB(FactDB* db);