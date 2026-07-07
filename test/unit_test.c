#include "greatest.h"

#include "../include/arena.h"
#include "../include/arena_internal.h"
#include "../include/factdb.h"
#include "../include/factdb_internal.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"
#include "../include/ActionEntry.h"
#include "../include/ActionEntry_internal.h"
#include "../include/rule.h"
#include "../include/rule_internal.h"
#include "../include/engine.h"
#include "../include/engine_internal.h"
#include "../include/parser_engine.h"
#include "../include/semanticChecker.h"

#include <math.h>
#include <float.h>

//-----------------------ARENA SUITE -------------------//

TEST arena_create_destroy(void){
    Arena* a = createArena(4096);
    ASSERT(a);
    ASSERT_EQ(4096, a->size);
    ASSERT_EQ(0, a->used);
    ASSERT(a->start);
    destroyArena(a);
    PASS();
}

TEST arena_alloc_basic(void){
    Arena* a = createArena(1024);
    ASSERT(a);
    int* p = (int*)arena_alloc(a, sizeof(int));
    ASSERT(p);
    *p = 42;
    ASSERT_EQ(42, *p);
    ASSERT(a->used > 0);
    destroyArena(a);
    PASS();
}

TEST arena_strdup_basic(void){
    Arena* a = createArena(2048);
    ASSERT(a);
    char* s = arena_strdup(a, "hello world");
    ASSERT(s);
    ASSERT_STR_EQ("hello world", s);
    destroyArena(a);
    PASS();
}

TEST arena_reset_reuse(void){
    Arena* a = createArena(2048);
    ASSERT(a);
    char* s1 = arena_strdup(a, "first");
    assert(s1);
    assert(a->used > 0);
    arena_reset(a);
    ASSERT_EQ(0, a->used);
    char* s2 = arena_strdup(a, "second");
    ASSERT(s2);
    ASSERT_STR_EQ("second", s2);
    destroyArena(a);
    PASS();
}

TEST arena_alloc_zero_size(void){
    Arena* a = createArena(1024);
    ASSERT(a);
    ASSERT_FALSE(arena_alloc(a, 0));
    destroyArena(a);
    PASS();
}


SUITE(arena_suite)
{
    RUN_TEST(arena_create_destroy);
    RUN_TEST(arena_alloc_basic);
    RUN_TEST(arena_strdup_basic);
    RUN_TEST(arena_reset_reuse);
    RUN_TEST(arena_alloc_zero_size);
}



// ---------------------- MAIN ------------------------//
GREATEST_MAIN_DEFS()

int main(int argc, char** argv)
{
    GREATEST_MAIN_BEGIN;
    RUN_SUITE(arena_suite);
    GREATEST_MAIN_END;
}
