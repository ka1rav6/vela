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


TEST arena_alloc_null_arena(void)
{
    ASSERT_FALSE(arena_alloc(NULL, 64));
    PASS();
}

TEST arena_overflow_returns_null(void)
{
    Arena* a = createArena(16);
    ASSERT(a);
    ASSERT(arena_alloc(a, 8));
    ASSERT_FALSE(arena_alloc(a, 16));
    destroyArena(a);
    PASS();
}

TEST arena_destroy_null(void)
{
    destroyArena(NULL);
    PASS();
}

TEST arena_reset_null(void)
{
    arena_reset(NULL);
    PASS();
}

TEST arena_null_create(void)
{
    ASSERT_FALSE(createArena(0));
    PASS();
}

SUITE(arena_suite)
{
    RUN_TEST(arena_create_destroy);
    RUN_TEST(arena_alloc_basic);
    RUN_TEST(arena_strdup_basic);
    RUN_TEST(arena_reset_reuse);
    RUN_TEST(arena_alloc_zero_size);
    RUN_TEST(arena_alloc_null_arena);
    RUN_TEST(arena_overflow_returns_null);
    RUN_TEST(arena_destroy_null);
    RUN_TEST(arena_reset_null);
    RUN_TEST(arena_null_create);
}



//--------------------FACTDB SUITE ---------------------//

TEST factdb_create_delete(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(db->boolFacts);
    ASSERT_FALSE(db->numFacts);
    ASSERT_FALSE(db->strFacts);
    deleteFactDB(db);
    PASS();
}

TEST factdb_set_get_bool(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    setBoolFact(db, "flag", true);
    ASSERT(getBoolFact(db, "flag"));
    setBoolFact(db, "flag", false);
    ASSERT_FALSE(getBoolFact(db, "flag"));
    deleteFactDB(db);
    PASS();
}

TEST factdb_set_get_num(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    setNumFact(db, "pi", 3.14159);
    ASSERT_EQ_FMT(3.14159, getNumFact(db, "pi"), "%.5f");
    setNumFact(db, "pi", 2.71828);
    ASSERT_EQ_FMT(2.71828, getNumFact(db, "pi"), "%.5f");
    deleteFactDB(db);
    PASS();
}

TEST factdb_set_get_str(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    setStringFact(db, "name", "Alice");
    char* v = getStringFact(db, "name");
    ASSERT(v);
    ASSERT_STR_EQ("Alice", v);
    setStringFact(db, "name", "Bob");
    v = getStringFact(db, "name");
    ASSERT_STR_EQ("Bob", v);
    deleteFactDB(db);
    PASS();
}

TEST factdb_get_missing_bool(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(getBoolFact(db, "nonexistent"));
    deleteFactDB(db);
    PASS();
}

TEST factdb_get_missing_num(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT(isnan(getNumFact(db, "nonexistent")));
    deleteFactDB(db);
    PASS();
}

TEST factdb_get_missing_str(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(getStringFact(db, "nonexistent"));
    deleteFactDB(db);
    PASS();
}

TEST factdb_has_fact_bool(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(factdb_has_fact(db, "flag", BOOL));
    setBoolFact(db, "flag", true);
    ASSERT(factdb_has_fact(db, "flag", BOOL));
    ASSERT_FALSE(factdb_has_fact(db, "flag", NUM));
    ASSERT_FALSE(factdb_has_fact(db, "flag", STR));
    deleteFactDB(db);
    PASS();
}

TEST factdb_has_fact_num(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(factdb_has_fact(db, "val", NUM));
    setNumFact(db, "val", 99.0);
    ASSERT(factdb_has_fact(db, "val", NUM));
    ASSERT_FALSE(factdb_has_fact(db, "val", BOOL));
    ASSERT_FALSE(factdb_has_fact(db, "val", STR));
    deleteFactDB(db);
    PASS();
}

TEST factdb_has_fact_str(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    ASSERT_FALSE(factdb_has_fact(db, "msg", STR));
    setStringFact(db, "msg", "hi");
    ASSERT(factdb_has_fact(db, "msg", STR));
    ASSERT_FALSE(factdb_has_fact(db, "msg", BOOL));
    ASSERT_FALSE(factdb_has_fact(db, "msg", NUM));
    deleteFactDB(db);
    PASS();
}

TEST factdb_mixed_types_independent(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    setBoolFact(db, "x", true);
    setNumFact(db, "x", 1.0);
    setStringFact(db, "x", "one");
    ASSERT(getBoolFact(db, "x"));
    ASSERT_EQ(1.0, getNumFact(db, "x"));
    ASSERT_STR_EQ("one", getStringFact(db, "x"));
    deleteFactDB(db);
    PASS();
}

TEST factdb_str_null_equivalence(void)
{
    FactDB* db = createFactDB();
    ASSERT(db);
    setStringFact(db, "empty", NULL);
    char* v = getStringFact(db, "empty");
    ASSERT_FALSE(v);
    deleteFactDB(db);
    PASS();
}

SUITE(factdb_suite)
{
    RUN_TEST(factdb_create_delete);
    RUN_TEST(factdb_set_get_bool);
    RUN_TEST(factdb_set_get_num);
    RUN_TEST(factdb_set_get_str);
    RUN_TEST(factdb_get_missing_bool);
    RUN_TEST(factdb_get_missing_num);
    RUN_TEST(factdb_get_missing_str);
    RUN_TEST(factdb_has_fact_bool);
    RUN_TEST(factdb_has_fact_num);
    RUN_TEST(factdb_has_fact_str);
    RUN_TEST(factdb_mixed_types_independent);
    RUN_TEST(factdb_str_null_equivalence);
}

//----------------BYTECODE/VM SUITE -------------------//

static FactDB* make_test_db(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "true_fact", true);
    setBoolFact(db, "false_fact", false);
    setNumFact(db, "age", 25);
    setNumFact(db, "score", 42);
    setNumFact(db, "zero", 0);
    setStringFact(db, "greeting", "hello");
    setStringFact(db, "empty_str", "");
    return db;
}

TEST vm_push_fact_true(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_FACT);
    n->data.Fact.factName = arena_strdup(a, "true_fact");
    Bytecode* bc = compileNode(a, n);
    ASSERT(bc);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_push_fact_false(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_FACT);
    n->data.Fact.factName = arena_strdup(a, "false_fact");
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_null_pushes_false(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_NULL);
    Bytecode* bc = compileNode(a, n);
    ASSERT(bc);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_gt(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "age");
    n->data.Compare.op = OP_GT;
    n->data.Compare.val = 18;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 30;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_ge(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "age");
    n->data.Compare.op = OP_GE;
    n->data.Compare.val = 25;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 26;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_lt(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "score");
    n->data.Compare.op = OP_LT;
    n->data.Compare.val = 100;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 10;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_le(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "score");
    n->data.Compare.op = OP_LE;
    n->data.Compare.val = 42;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 41;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_eq(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "score");
    n->data.Compare.op = OP_EQ;
    n->data.Compare.val = 42;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 0;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_compare_ne(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(a, "score");
    n->data.Compare.op = OP_NE;
    n->data.Compare.val = 0;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.Compare.val = 42;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_str_cmp_eq(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_STR_CMP);
    n->data.StrCmp.factName = arena_strdup(a, "greeting");
    n->data.StrCmp.strVal = arena_strdup(a, "hello");
    n->data.StrCmp.op = OP_EQ;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.StrCmp.strVal = arena_strdup(a, "world");
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_str_cmp_ne(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_STR_CMP);
    n->data.StrCmp.factName = arena_strdup(a, "greeting");
    n->data.StrCmp.strVal = arena_strdup(a, "world");
    n->data.StrCmp.op = OP_NE;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    n->data.StrCmp.strVal = arena_strdup(a, "hello");
    bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_str_cmp_empty_with_empty(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_STR_CMP);
    n->data.StrCmp.factName = arena_strdup(a, "empty_str");
    n->data.StrCmp.strVal = arena_strdup(a, "");
    n->data.StrCmp.op = OP_EQ;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_str_cmp_missing_fact(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* n = createNode(a, NODE_STR_CMP);
    n->data.StrCmp.factName = arena_strdup(a, "no_such_fact");
    n->data.StrCmp.strVal = arena_strdup(a, "x");
    n->data.StrCmp.op = OP_EQ;
    Bytecode* bc = compileNode(a, n);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    n->data.StrCmp.op = OP_NE;
    bc = compileNode(a, n);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_and_true_true(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* l = createNode(a, NODE_FACT);
    l->data.Fact.factName = arena_strdup(a, "true_fact");
    Node* r = createNode(a, NODE_FACT);
    r->data.Fact.factName = arena_strdup(a, "true_fact");
    Node* andN = createNode(a, NODE_AND);
    andN->data.op.left = l;
    andN->data.op.right = r;

    Bytecode* bc = compileNode(a, andN);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_and_true_false(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* l = createNode(a, NODE_FACT);
    l->data.Fact.factName = arena_strdup(a, "true_fact");
    Node* r = createNode(a, NODE_FACT);
    r->data.Fact.factName = arena_strdup(a, "false_fact");
    Node* andN = createNode(a, NODE_AND);
    andN->data.op.left = l;
    andN->data.op.right = r;

    Bytecode* bc = compileNode(a, andN);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_or_false_true(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* l = createNode(a, NODE_FACT);
    l->data.Fact.factName = arena_strdup(a, "false_fact");
    Node* r = createNode(a, NODE_FACT);
    r->data.Fact.factName = arena_strdup(a, "true_fact");
    Node* orN = createNode(a, NODE_OR);
    orN->data.op.left = l;
    orN->data.op.right = r;

    Bytecode* bc = compileNode(a, orN);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_or_false_false(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* l = createNode(a, NODE_FACT);
    l->data.Fact.factName = arena_strdup(a, "false_fact");
    Node* r = createNode(a, NODE_FACT);
    r->data.Fact.factName = arena_strdup(a, "false_fact");
    Node* orN = createNode(a, NODE_OR);
    orN->data.op.left = l;
    orN->data.op.right = r;

    Bytecode* bc = compileNode(a, orN);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_not_true(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* c = createNode(a, NODE_FACT);
    c->data.Fact.factName = arena_strdup(a, "true_fact");
    Node* notN = createNode(a, NODE_NOT);
    notN->data.unary.child = c;

    Bytecode* bc = compileNode(a, notN);
    ASSERT_EQ(VM_FALSE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_not_false(void)
{
    Arena* a = createArena(2048);
    FactDB* db = make_test_db();

    Node* c = createNode(a, NODE_FACT);
    c->data.Fact.factName = arena_strdup(a, "false_fact");
    Node* notN = createNode(a, NODE_NOT);
    notN->data.unary.child = c;

    Bytecode* bc = compileNode(a, notN);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_complex_nested(void)
{
    Arena* a = createArena(1024);
    FactDB* db = make_test_db();

    /* (age > 18) AND (isAdmin OR isGuest) */
    Node* cmp = createNode(a, NODE_COMPARE);
    cmp->data.Compare.factName = arena_strdup(a, "age");
    cmp->data.Compare.op = OP_GT;
    cmp->data.Compare.val = 18;

    Node* admin = createNode(a, NODE_FACT);
    admin->data.Fact.factName = arena_strdup(a, "true_fact");

    Node* guest = createNode(a, NODE_FACT);
    guest->data.Fact.factName = arena_strdup(a, "false_fact");

    Node* orN = createNode(a, NODE_OR);
    orN->data.op.left = admin;
    orN->data.op.right = guest;

    Node* andN = createNode(a, NODE_AND);
    andN->data.op.left = cmp;
    andN->data.op.right = orN;

    Bytecode* bc = compileNode(a, andN);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

TEST vm_nested_not_and_or(void)
{
    Arena* a = createArena(1024);
    FactDB* db = make_test_db();

    /* NOT( age <= 18 OR isBanned ) */
    Node* cmp = createNode(a, NODE_COMPARE);
    cmp->data.Compare.factName = arena_strdup(a, "age");
    cmp->data.Compare.op = OP_LE;
    cmp->data.Compare.val = 18;

    Node* banned = createNode(a, NODE_FACT);
    banned->data.Fact.factName = arena_strdup(a, "false_fact");

    Node* orN = createNode(a, NODE_OR);
    orN->data.op.left = cmp;
    orN->data.op.right = banned;

    Node* notN = createNode(a, NODE_NOT);
    notN->data.unary.child = orN;

    Bytecode* bc = compileNode(a, notN);
    ASSERT_EQ(VM_TRUE, runBytecode(db, bc));

    destroyArena(a);
    deleteFactDB(db);
    PASS();
}

SUITE(vm_suite)
{
    RUN_TEST(vm_push_fact_true);
    RUN_TEST(vm_push_fact_false);
    RUN_TEST(vm_null_pushes_false);
    RUN_TEST(vm_compare_gt);
    RUN_TEST(vm_compare_ge);
    RUN_TEST(vm_compare_lt);
    RUN_TEST(vm_compare_le);
    RUN_TEST(vm_compare_eq);
    RUN_TEST(vm_compare_ne);
    RUN_TEST(vm_str_cmp_eq);
    RUN_TEST(vm_str_cmp_ne);
    RUN_TEST(vm_str_cmp_empty_with_empty);
    RUN_TEST(vm_str_cmp_missing_fact);
    RUN_TEST(vm_and_true_true);
    RUN_TEST(vm_and_true_false);
    RUN_TEST(vm_or_false_true);
    RUN_TEST(vm_or_false_false);
    RUN_TEST(vm_not_true);
    RUN_TEST(vm_not_false);
    RUN_TEST(vm_complex_nested);
    RUN_TEST(vm_nested_not_and_or);
}










// ---------------------- MAIN ------------------------//
GREATEST_MAIN_DEFS()

int main(int argc, char** argv)
{
    GREATEST_MAIN_BEGIN;
    RUN_SUITE(arena_suite);
    RUN_SUITE(factdb_suite);
    RUN_SUITE(vm_suite);
    GREATEST_MAIN_END;
}
