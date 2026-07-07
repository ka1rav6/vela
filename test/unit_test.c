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


//----------------- ACTIONENTRY SUITE-------------------//

static int action_called_count;
static void* action_called_ctx;

static void test_action_fn(FactDB* db, void* ctx)
{
    (void)db;
    action_called_count++;
    action_called_ctx = ctx;
}

TEST action_register_lookup(void)
{
    ActionEntry* reg = NULL;
    ASSERT_EQ(0, registerAction(&reg, "my_action", test_action_fn, (void*)0x42));
    ASSERT(reg);

    ActionEntry* e = lookupAction(reg, "my_action");
    ASSERT(e);
    ASSERT_STR_EQ("my_action", e->action);
    ASSERT_EQ(test_action_fn, e->func);
    ASSERT_EQ((void*)0x42, e->ctx);

    freeRegistry(&reg);
    ASSERT_FALSE(reg);
    PASS();
}

TEST action_accessor_func(void)
{
    ActionEntry* reg = NULL;
    registerAction(&reg, "act", test_action_fn, (void*)0x99);
    ActionEntry* e = lookupAction(reg, "act");
    ASSERT_EQ(test_action_fn, action_entry_func(e));
    ASSERT_EQ((void*)0x99, action_entry_ctx(e));
    freeRegistry(&reg);
    PASS();
}

TEST action_accessor_null(void)
{
    ASSERT_FALSE(action_entry_func(NULL));
    ASSERT_FALSE(action_entry_ctx(NULL));
    PASS();
}

TEST action_lookup_missing(void)
{
    ActionEntry* reg = NULL;
    registerAction(&reg, "existing", test_action_fn, NULL);
    ASSERT_FALSE(lookupAction(reg, "nonexistent"));
    freeRegistry(&reg);
    PASS();
}

TEST action_name_too_long(void)
{
    ActionEntry* reg = NULL;
    char longname[MAX_ACTION_NAME + 2];
    memset(longname, 'A', sizeof(longname) - 1);
    longname[sizeof(longname) - 1] = '\0';
    ASSERT_EQ(-1, registerAction(&reg, longname, test_action_fn, NULL));
    freeRegistry(&reg);
    PASS();
}

SUITE(action_suite)
{
    RUN_TEST(action_register_lookup);
    RUN_TEST(action_accessor_func);
    RUN_TEST(action_accessor_null);
    RUN_TEST(action_lookup_missing);
    RUN_TEST(action_name_too_long);
}

//----------------SEMANTIC CHECKER SUITE----------------//

TEST sem_is_operator(void)
{
    ASSERT(isOperator(">="));
    ASSERT(isOperator("<="));
    ASSERT(isOperator("=="));
    ASSERT(isOperator("!="));
    ASSERT(isOperator(">"));
    ASSERT(isOperator("<"));
    ASSERT(isOperator("and"));
    ASSERT(isOperator("or"));
    ASSERT(isOperator("not"));
    ASSERT(isOperator("null"));
    ASSERT_FALSE(isOperator("xor"));
    ASSERT_FALSE(isOperator("&&"));
    ASSERT_FALSE(isOperator(""));
    PASS();
}

TEST sem_is_comparison_correct(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "flag", true);
    setNumFact(db, "age", 25);
    ASSERT_FALSE(isComparisonCorrect(db, "flag"));
    ASSERT(isComparisonCorrect(db, "age"));
    ASSERT(isComparisonCorrect(db, "nonexistent"));
    deleteFactDB(db);
    PASS();
}

TEST sem_fact_exists(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "b", true);
    setNumFact(db, "n", 1.0);
    setStringFact(db, "s", "x");
    ASSERT(factExists(db, "b", BOOL));
    ASSERT_FALSE(factExists(db, "b", NUM));
    ASSERT_FALSE(factExists(db, "b", STR));
    ASSERT(factExists(db, "n", NUM));
    ASSERT(factExists(db, "s", STR));
    ASSERT_FALSE(factExists(db, "z", BOOL));
    deleteFactDB(db);
    PASS();
}

TEST sem_duplicate_rule(void)
{
    RuleEngine* re = createRuleEngine();
    ASSERT(re);
    Arena* a = re->arena;
    ASSERT_FALSE(duplicateRule(re, "r1"));

    Node* n = createNode(a, NODE_FACT);
    n->data.Fact.factName = arena_strdup(a, "true_fact");
    Rule* r = createRule(re, n, "A", "r1", NULL);
    addRule(re, r);

    ASSERT(duplicateRule(re, "r1"));
    ASSERT_FALSE(duplicateRule(re, "r2"));
    ASSERT_FALSE(duplicateRule(re, "R1"));

    deleteRuleEngine(re);
    PASS();
}

TEST sem_is_empty_array(void)
{
    yyjson_doc* doc = yyjson_read("[1, 2]", strlen("[1, 2]"), 0);
    yyjson_val* arr = yyjson_doc_get_root(doc);
    ASSERT_FALSE(isEmptyOrUndersizedArray(arr, "and"));

    yyjson_doc* emptyDoc = yyjson_read("[]", strlen("[]"), 0);
    yyjson_val* emptyArr = yyjson_doc_get_root(emptyDoc);
    ASSERT(isEmptyOrUndersizedArray(emptyArr, "and"));

    yyjson_doc* singleDoc = yyjson_read("[42]", strlen("[42]"), 0);
    yyjson_val* singleArr = yyjson_doc_get_root(singleDoc);
    ASSERT(isEmptyOrUndersizedArray(singleArr, "and"));
    ASSERT_FALSE(isEmptyOrUndersizedArray(singleArr, "not"));

    yyjson_doc_free(doc);
    yyjson_doc_free(emptyDoc);
    yyjson_doc_free(singleDoc);
    PASS();
}

TEST sem_is_mixed_types(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "b", true);
    setNumFact(db, "n", 1.0);

    yyjson_doc* doc = yyjson_read("[\"b\", \"n\"]", strlen("[\"b\", \"n\"]"), 0);
    yyjson_val* arr = yyjson_doc_get_root(doc);
    EngineError err = ENGINE_SUCCESS;
    ASSERT(isMixedBoolNumArray(db, arr, &err));
    ASSERT_EQ(ENGINE_ERR_BOOL_COMPARED, err);

    yyjson_doc_free(doc);
    deleteFactDB(db);
    PASS();
}

TEST sem_is_mixed_same_type_ok(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "b1", true);
    setBoolFact(db, "b2", false);

    yyjson_doc* doc = yyjson_read("[\"b1\", \"b2\"]", strlen("[\"b1\", \"b2\"]"), 0);
    yyjson_val* arr = yyjson_doc_get_root(doc);
    ASSERT_FALSE(isMixedBoolNumArray(db, arr, NULL));

    yyjson_doc_free(doc);
    deleteFactDB(db);
    PASS();
}

TEST sem_is_mixed_null_err(void)
{
    FactDB* db = createFactDB();
    yyjson_doc* doc = yyjson_read("[\"x\", \"y\"]", strlen("[\"x\", \"y\"]"), 0);
    yyjson_val* arr = yyjson_doc_get_root(doc);
    ASSERT_FALSE(isMixedBoolNumArray(db, arr, NULL));
    yyjson_doc_free(doc);
    deleteFactDB(db);
    PASS();
}

SUITE(semantic_suite)
{
    RUN_TEST(sem_is_operator);
    RUN_TEST(sem_is_comparison_correct);
    RUN_TEST(sem_fact_exists);
    RUN_TEST(sem_duplicate_rule);
    RUN_TEST(sem_is_empty_array);
    RUN_TEST(sem_is_mixed_types);
    RUN_TEST(sem_is_mixed_same_type_ok);
    RUN_TEST(sem_is_mixed_null_err);
}

//--------------ENGINE INTEGRATION SUITE --------------//

TEST engine_strerror_all_codes(void)
{
    ASSERT_STR_EQ("success", engine_strerror(ENGINE_SUCCESS));
    ASSERT_STR_EQ("null argument", engine_strerror(ENGINE_ERR_NULL_ARG));
    ASSERT_STR_EQ("cannot open file", engine_strerror(ENGINE_ERR_CANT_OPEN_FILE));
    ASSERT_STR_EQ("file too small", engine_strerror(ENGINE_ERR_FILE_TOO_SMALL));
    ASSERT_STR_EQ("invalid JSON", engine_strerror(ENGINE_ERR_INVALID_JSON));
    ASSERT_STR_EQ("out of memory", engine_strerror(ENGINE_ERR_OUT_OF_MEMORY));
    ASSERT_STR_EQ("arena out of memory", engine_strerror(ENGINE_ERR_ARENA_OOM));
    ASSERT_STR_EQ("mmap failed", engine_strerror(ENGINE_ERR_MMAP));
    ASSERT_STR_EQ("munmap failed", engine_strerror(ENGINE_ERR_MUNMAP));
    ASSERT_STR_EQ("mutex init failed", engine_strerror(ENGINE_ERR_MUTEX));
    ASSERT_STR_EQ("duplicate rule name", engine_strerror(ENGINE_ERR_DUPLICATE_RULE));
    ASSERT_STR_EQ("invalid operator", engine_strerror(ENGINE_ERR_INVALID_OPERATOR));
    ASSERT_STR_EQ("rule missing name", engine_strerror(ENGINE_ERR_MISSING_RULE_NAME));
    ASSERT_STR_EQ("rule missing action", engine_strerror(ENGINE_ERR_MISSING_RULE_ACTION));
    ASSERT_STR_EQ("fact not found", engine_strerror(ENGINE_ERR_FACT_NOT_FOUND));
    ASSERT_STR_EQ("bool compared with number", engine_strerror(ENGINE_ERR_BOOL_COMPARED));
    ASSERT_STR_EQ("empty operator array", engine_strerror(ENGINE_ERR_EMPTY_ARRAY));
    ASSERT_STR_EQ("invalid comparison value", engine_strerror(ENGINE_ERR_INVALID_VALUE));
    ASSERT_STR_EQ("invalid bytecode magic", engine_strerror(ENGINE_ERR_INVALID_MAGIC));
    ASSERT_STR_EQ("invalid bytecode version", engine_strerror(ENGINE_ERR_INVALID_VERSION));
    ASSERT_STR_EQ("invalid rule count", engine_strerror(ENGINE_ERR_INVALID_RULE_COUNT));
    ASSERT_STR_EQ("invalid instruction count", engine_strerror(ENGINE_ERR_INVALID_INSTR_COUNT));
    ASSERT_STR_EQ("truncated bytecode file", engine_strerror(ENGINE_ERR_TRUNCATED_FILE));
    ASSERT_STR_EQ("instruction count mismatch", engine_strerror(ENGINE_ERR_INSTRUCTION_COUNT_MISMATCH));
    ASSERT_STR_EQ("action name too long", engine_strerror(ENGINE_ERR_ACTION_NAME_TOO_LONG));
    ASSERT_STR_EQ("fact name too long", engine_strerror(ENGINE_ERR_FACT_NAME_TOO_LONG));
    ASSERT_STR_EQ("rule name too long", engine_strerror(ENGINE_ERR_RULE_NAME_TOO_LONG));
    ASSERT_STR_EQ("VM stack overflow", engine_strerror(ENGINE_ERR_STACK_OVERFLOW));
    ASSERT_STR_EQ("VM stack underflow", engine_strerror(ENGINE_ERR_STACK_UNDERFLOW));
    ASSERT_STR_EQ("VM execution error", engine_strerror(ENGINE_ERR_VM));
    ASSERT_STR_EQ("parse error", engine_strerror(ENGINE_ERR_PARSE));
    ASSERT_STR_EQ("string comparison only supports == and !=", engine_strerror(ENGINE_ERR_STRING_CMP_NOT_EQ_NE));
    ASSERT_STR_EQ("unknown error", engine_strerror((EngineError)9999));
    PASS();
}

TEST engine_invalid_file_returns_error(void)
{
    Engine* e = createEngine("/nonexistent/path.json", JSON);
    ASSERT_FALSE(e);
    PASS();
}

TEST engine_get_last_error_null(void)
{
    ASSERT_EQ(ENGINE_ERR_NULL_ARG, engine_get_last_error(NULL));
    PASS();
}

SUITE(engine_suite)
{
    RUN_TEST(engine_strerror_all_codes);
    RUN_TEST(engine_invalid_file_returns_error);
    RUN_TEST(engine_get_last_error_null);
}

//--------ENGINE INTEGRATION LOW LEVEL SUITE-----------//

TEST rule_engine_create_destroy(void)
{
    RuleEngine* re = createRuleEngine();
    ASSERT(re);
    ASSERT(re->arena);
    ASSERT_FALSE(re->rules);
    deleteRuleEngine(re);
    PASS();
}

TEST rule_engine_create_rule_add_find(void)
{
    RuleEngine* re = createRuleEngine();
    ASSERT(re);

    Node* n = createNode(re->arena, NODE_NULL);
    Rule* r = createRule(re, n, "MY_ACTION", "my_rule", NULL);
    ASSERT(r);
    ASSERT_STR_EQ("my_rule", rule_name(r));
    ASSERT_STR_EQ("MY_ACTION", rule_action(r));
    ASSERT_EQ(n, rule_condition(r));

    addRule(re, r);
    Rule* found = findRule(re, "my_rule");
    ASSERT(found);
    ASSERT_EQ(r, found);

    ASSERT_FALSE(findRule(re, "nonexistent"));
    deleteRuleEngine(re);
    PASS();
}

TEST re_bind_action(void)
{
    RuleEngine* re = createRuleEngine();
    ASSERT(re);

    Node* n1 = createNode(re->arena, NODE_NULL);
    Node* n2 = createNode(re->arena, NODE_NULL);
    Rule* r1 = createRule(re, n1, "A", "r1", NULL);
    Rule* r2 = createRule(re, n2, "A", "r2", (void*)0x1);
    Rule* r3 = createRule(re, createNode(re->arena, NODE_NULL), "B", "r3", (void*)0x2);
    addRule(re, r1);
    addRule(re, r2);
    addRule(re, r3);

    int ctx_val = 0;
    rule_engine_bind_action(re, "A", test_action_fn, &ctx_val);
    ASSERT_EQ(test_action_fn, r1->func);
    ASSERT_EQ(&ctx_val, r1->ctx);
    ASSERT_EQ(test_action_fn, r2->func);
    ASSERT_EQ(&ctx_val, r2->ctx);
    ASSERT_FALSE(r3->func);

    deleteRuleEngine(re);
    PASS();
}

static void re_for_each_cb(Rule* r, void* ctx)
{
    (void)r;
    (*(int*)ctx)++;
}

TEST re_for_each(void)
{
    RuleEngine* re = createRuleEngine();
    int count = 0;

    for (int i = 0; i < 3; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "r%d", i);
        Rule* r = createRule(re, createNode(re->arena, NODE_NULL), "A", name, NULL);
        addRule(re, r);
    }

    rule_engine_for_each(re, re_for_each_cb, &count);
    ASSERT_EQ(3, count);
    deleteRuleEngine(re);
    PASS();
}

TEST rule_engine_delete_null(void)
{
    deleteRuleEngine(NULL);
    PASS();
}

SUITE(rule_engine_suite)
{
    RUN_TEST(rule_engine_create_destroy);
    RUN_TEST(rule_engine_create_rule_add_find);
    RUN_TEST(re_bind_action);
    RUN_TEST(re_for_each);
    RUN_TEST(rule_engine_delete_null);
}


//-------------------PARSER SUITE ---------------------//

TEST parse_json_file_not_found(void)
{
    ASSERT_FALSE(parseJSON("/nonexistent/file.json"));
    PASS();
}

TEST parse_json_valid_file(void)
{
    yyjson_doc* doc = parseJSON("../test/test1.json");
    ASSERT(doc);
    yyjson_doc_free(doc);
    PASS();
}

TEST build_ast_with_nonexistent_fact(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "exists", true);

    const char* json = "{\"facts\":{\"exists\":true},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":\"nonexistent\"}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_FACT_NOT_FOUND, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_missing_rule_name(void)
{
    FactDB* db = createFactDB();
    const char* json = "{\"facts\":{},\"rules\":["
        "{\"action\":\"A\",\"if\":\"_\"}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_MISSING_RULE_NAME, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_invalid_operator(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "x", true);
    const char* json = "{\"facts\":{\"x\":true},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\"xor\":[\"x\"]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_INVALID_OPERATOR, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_empty_and(void)
{
    FactDB* db = createFactDB();
    const char* json = "{\"facts\":{},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\"and\":[]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_EMPTY_ARRAY, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_duplicate_rule(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "x", true);
    const char* json = "{\"facts\":{\"x\":true},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":\"x\"},"
        "{\"name\":\"r\",\"action\":\"A\",\"if\":\"x\"}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_DUPLICATE_RULE, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_null_node(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "x", true);
    setBoolFact(db, "y", true);
    const char* json = "{\"facts\":{\"x\":true,\"y\":true},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\"and\":[\"x\",{\"null\":[]}]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT(re);
    ASSERT_EQ(ENGINE_SUCCESS, err);

    Rule* r = findRule(re, "r");
    ASSERT(r);
    ASSERT_EQ(NODE_AND, r->condition->type);

    deleteRuleEngine(re);
    deleteFactDB(db);
    PASS();
}

TEST build_ast_str_cmp(void)
{
    FactDB* db = createFactDB();
    setStringFact(db, "role", "admin");
    const char* json = "{\"facts\":{\"role\":\"admin\"},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\"==\":[\"role\",\"admin\"]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT(re);
    ASSERT_EQ(ENGINE_SUCCESS, err);

    Rule* r = findRule(re, "r");
    ASSERT(r);

    VMResult res = runBytecode(db, r->bc);
    ASSERT_EQ(VM_TRUE, res);

    deleteRuleEngine(re);
    deleteFactDB(db);
    PASS();
}

TEST build_ast_bool_compared_error(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "flag", true);
    const char* json = "{\"facts\":{\"flag\":true},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\">\":[\"flag\",0]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_BOOL_COMPARED, err);

    deleteFactDB(db);
    PASS();
}

TEST build_ast_mixed_bool_num_array(void)
{
    FactDB* db = createFactDB();
    setBoolFact(db, "b", true);
    setNumFact(db, "n", 1.0);
    const char* json = "{\"facts\":{\"b\":true,\"n\":1.0},\"rules\":["
        "{\"name\":\"r\",\"action\":\"A\",\"if\":{\"and\":[\"b\",\"n\"]}}]}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    ASSERT(doc);

    EngineError err = ENGINE_SUCCESS;
    RuleEngine* re = build_ast(doc, db, &err);
    ASSERT_FALSE(re);
    ASSERT_EQ(ENGINE_ERR_BOOL_COMPARED, err);

    deleteFactDB(db);
    PASS();
}

SUITE(parser_suite)
{
    RUN_TEST(parse_json_file_not_found);
    RUN_TEST(parse_json_valid_file);
    RUN_TEST(build_ast_with_nonexistent_fact);
    RUN_TEST(build_ast_missing_rule_name);
    RUN_TEST(build_ast_invalid_operator);
    RUN_TEST(build_ast_empty_and);
    RUN_TEST(build_ast_duplicate_rule);
    RUN_TEST(build_ast_null_node);
    RUN_TEST(build_ast_str_cmp);
    RUN_TEST(build_ast_bool_compared_error);
    RUN_TEST(build_ast_mixed_bool_num_array);
}


//---------------FULL ENIGNE INTEGRATION---------------//

TEST engine_full_integration_json(void)
{
    Engine* e = createEngine("../test/test1.json", JSON);
    ASSERT(e);
    ASSERT_EQ(ENGINE_SUCCESS, engine_get_last_error(e));

    int pass_count = 0;
    rule_engine_bind_action(engine_get_rule_engine(e), "SIMPLE_BOOL_FIRED",
        test_action_fn, &pass_count);

    EngineError err = runEngine(e);
    ASSERT_EQ(ENGINE_SUCCESS, err);

    deleteEngine(e);
    PASS();
}

TEST engine_create_with_null_file(void)
{
    Engine* e = createEngine(NULL, JSON);
    ASSERT_FALSE(e);
    PASS();
}

TEST engine_register_null_args(void)
{
    Engine* e = createEngine("../test/test1.json", JSON);
    ASSERT(e);
    ASSERT_EQ(ENGINE_ERR_NULL_ARG, registerTheAction(NULL, "x", test_action_fn, NULL));
    ASSERT_EQ(ENGINE_ERR_NULL_ARG, registerTheAction(e, NULL, test_action_fn, NULL));
    ASSERT_EQ(ENGINE_ERR_NULL_ARG, registerTheAction(e, "x", NULL, NULL));
    deleteEngine(e);
    PASS();
}

SUITE(full_integration_suite)
{
    RUN_TEST(engine_full_integration_json);
    RUN_TEST(engine_create_with_null_file);
    RUN_TEST(engine_register_null_args);
}




// ---------------------- MAIN ------------------------//
GREATEST_MAIN_DEFS()

int main(int argc, char** argv)
{
    GREATEST_MAIN_BEGIN;
    RUN_SUITE(arena_suite);
    RUN_SUITE(factdb_suite);
    RUN_SUITE(vm_suite);
    RUN_SUITE(action_suite);
    RUN_SUITE(semantic_suite);
    RUN_SUITE(engine_suite);
    RUN_SUITE(rule_engine_suite);
    RUN_SUITE(parser_suite);
    RUN_SUITE(full_integration_suite);
    GREATEST_MAIN_END;
}
