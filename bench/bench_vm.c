#include "bench.h"
#include "../include/factdb.h"
#include "../include/arena.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"

static FactDB* db_simple;
static FactDB* db_all;

static void setup_facts(void)
{
    db_simple = createFactDB();
    setBoolFact(db_simple, "flag", true);

    db_all = createFactDB();
    setBoolFact(db_all, "t1", true);
    setBoolFact(db_all, "t2", true);
    setBoolFact(db_all, "f1", false);
    setBoolFact(db_all, "f2", false);
    setBoolFact(db_all, "t", true);
    setBoolFact(db_all, "f", false);
    setNumFact(db_all, "age", 25);
    setNumFact(db_all, "score", 42);
    setNumFact(db_all, "balance", 1500);
}

static void teardown_facts(void)
{
    deleteFactDB(db_simple);
    deleteFactDB(db_all);
}

static double bench_simple_fact(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* n = createNode(ar, NODE_FACT);
    n->data.Fact.factName = arena_strdup(ar, "flag");
    Bytecode* bc = compileNode(ar, n);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_simple, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_and(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* l = createNode(ar, NODE_FACT);
    l->data.Fact.factName = arena_strdup(ar, "t1");
    Node* r = createNode(ar, NODE_FACT);
    r->data.Fact.factName = arena_strdup(ar, "f1");
    Node* andN = createNode(ar, NODE_AND);
    andN->data.op.left = l;
    andN->data.op.right = r;
    Bytecode* bc = compileNode(ar, andN);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_or(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* l = createNode(ar, NODE_FACT);
    l->data.Fact.factName = arena_strdup(ar, "f1");
    Node* r = createNode(ar, NODE_FACT);
    r->data.Fact.factName = arena_strdup(ar, "f2");
    Node* orN = createNode(ar, NODE_OR);
    orN->data.op.left = l;
    orN->data.op.right = r;
    Bytecode* bc = compileNode(ar, orN);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_not(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* c = createNode(ar, NODE_FACT);
    c->data.Fact.factName = arena_strdup(ar, "t");
    Node* notN = createNode(ar, NODE_NOT);
    notN->data.unary.child = c;
    Bytecode* bc = compileNode(ar, notN);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_compare_gt(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* n = createNode(ar, NODE_COMPARE);
    n->data.Compare.factName = arena_strdup(ar, "age");
    n->data.Compare.op = OP_GT;
    n->data.Compare.val = 18;
    Bytecode* bc = compileNode(ar, n);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_complex(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* cmp = createNode(ar, NODE_COMPARE);
    cmp->data.Compare.factName = arena_strdup(ar, "age");
    cmp->data.Compare.op = OP_GT;
    cmp->data.Compare.val = 18;
    Node* admin = createNode(ar, NODE_FACT);
    admin->data.Fact.factName = arena_strdup(ar, "t");
    Node* orN = createNode(ar, NODE_OR);
    orN->data.op.left = cmp;
    orN->data.op.right = admin;
    Node* bal = createNode(ar, NODE_COMPARE);
    bal->data.Compare.factName = arena_strdup(ar, "balance");
    bal->data.Compare.op = OP_GE;
    bal->data.Compare.val = 500;
    Node* andN = createNode(ar, NODE_AND);
    andN->data.op.left = orN;
    andN->data.op.right = bal;
    Bytecode* bc = compileNode(ar, andN);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

static double bench_10node(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    Node* nodes[5];
    for (int i = 0; i < 5; i++) {
        nodes[i] = createNode(ar, NODE_FACT);
        nodes[i]->data.Fact.factName = arena_strdup(ar, (i % 2) ? "t" : "f");
    }
    Node* a1 = createNode(ar, NODE_AND);
    a1->data.op.left = nodes[0];
    a1->data.op.right = nodes[1];
    Node* a2 = createNode(ar, NODE_AND);
    a2->data.op.left = a1;
    a2->data.op.right = nodes[2];
    Node* o1 = createNode(ar, NODE_OR);
    o1->data.op.left = a2;
    o1->data.op.right = nodes[3];
    Node* n1 = createNode(ar, NODE_NOT);
    n1->data.unary.child = o1;
    Node* a3 = createNode(ar, NODE_AND);
    a3->data.op.left = n1;
    a3->data.op.right = nodes[4];
    Bytecode* bc = compileNode(ar, a3);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db_all, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    destroyArena(ar);
    return elapsed;
}

void bench_vm(BenchContext* ctx)
{
    setup_facts();

    double ns, elapsed;
    long mem;
    uint64_t it;

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_simple_fact, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_simple_fact", "VM", "Single bool fact push", ns, elapsed, it, mem, 1, 1, 0, 0);

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_and, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_and", "VM", "AND of two bools (true && false)", ns, elapsed, it, mem, 1, 2, 0, 2);

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_or, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_or", "VM", "OR of two bools (false || false)", ns, elapsed, it, mem, 1, 2, 0, 2);

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_not, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_not", "VM", "NOT of a bool fact", ns, elapsed, it, mem, 1, 1, 0, 1);

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_compare_gt, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_compare_gt", "VM", "Numeric greater-than comparison", ns, elapsed, it, mem, 1, 1, 0, 0);

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_complex, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_complex_nested", "VM", "Deep nested AND/OR/CMP (5 nodes)", ns, elapsed, it, mem, 1, 3, 0, 5);

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_10node, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "vm_10node_tree", "VM", "10-node condition tree", ns, elapsed, it, mem, 1, 2, 0, 10);

    teardown_facts();
}
