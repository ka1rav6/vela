#include "bench.h"
#include "../include/factdb.h"
#include "../include/arena.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"

static bool walk_eval(FactDB* db, Node* n)
{
    if (!n) return false;
    switch (n->type) {
        case NODE_FACT:
            return getBoolFact(db, n->data.Fact.factName);
        case NODE_COMPARE: {
            double val = getNumFact(db, n->data.Compare.factName);
            switch (n->data.Compare.op) {
                case OP_GT: return val > n->data.Compare.val;
                case OP_LT: return val < n->data.Compare.val;
                case OP_GE: return val >= n->data.Compare.val;
                case OP_LE: return val <= n->data.Compare.val;
                case OP_EQ: return val == n->data.Compare.val;
                case OP_NE: return val != n->data.Compare.val;
                default: return false;
            }
        }
        case NODE_AND:
            return walk_eval(db, n->data.op.left) && walk_eval(db, n->data.op.right);
        case NODE_OR:
            return walk_eval(db, n->data.op.left) || walk_eval(db, n->data.op.right);
        case NODE_NOT:
            return !walk_eval(db, n->data.unary.child);
        case NODE_NULL:
            return false;
        default:
            return false;
    }
}

static Node* build_big_tree(Arena* ar, int depth)
{
    if (depth <= 0) {
        Node* n = createNode(ar, NODE_FACT);
        n->data.Fact.factName = arena_strdup(ar, "t");
        return n;
    }
    Node* l = build_big_tree(ar, depth - 1);
    Node* r = build_big_tree(ar, depth - 1);
    Node* andN = createNode(ar, NODE_AND);
    andN->data.op.left = l;
    andN->data.op.right = r;
    return andN;
}

static double bench_tree_walk_big(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    FactDB* db = createFactDB();
    setBoolFact(db, "t", true);
    Node* tree = build_big_tree(ar, 10);
    bool res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = walk_eval(db, tree);
    double elapsed = bench_time_ns() - start;
    (void)res;
    deleteFactDB(db);
    destroyArena(ar);
    return elapsed;
}

static double bench_bytecode_big(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    FactDB* db = createFactDB();
    setBoolFact(db, "t", true);
    Node* tree = build_big_tree(ar, 10);
    Bytecode* bc = compileNode(ar, tree);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    deleteFactDB(db);
    destroyArena(ar);
    return elapsed;
}

static Node* build_mixed_tree(Arena* ar)
{
    Node* f = createNode(ar, NODE_FACT);
    f->data.Fact.factName = arena_strdup(ar, "t");
    Node* c = createNode(ar, NODE_COMPARE);
    c->data.Compare.factName = arena_strdup(ar, "n");
    c->data.Compare.op = OP_GT;
    c->data.Compare.val = 18;
    Node* a = createNode(ar, NODE_AND);
    a->data.op.left = f;
    a->data.op.right = c;
    Node* no = createNode(ar, NODE_NOT);
    no->data.unary.child = a;
    Node* o = createNode(ar, NODE_OR);
    Node* f2 = createNode(ar, NODE_FACT);
    f2->data.Fact.factName = arena_strdup(ar, "t");
    o->data.op.left = no;
    o->data.op.right = f2;
    return o;
}

static double bench_tree_walk_mixed(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    FactDB* db = createFactDB();
    setBoolFact(db, "t", true);
    setNumFact(db, "n", 42);
    Node* tree = build_mixed_tree(ar);
    bool res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = walk_eval(db, tree);
    double elapsed = bench_time_ns() - start;
    (void)res;
    deleteFactDB(db);
    destroyArena(ar);
    return elapsed;
}

static double bench_bytecode_mixed(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    FactDB* db = createFactDB();
    setBoolFact(db, "t", true);
    setNumFact(db, "n", 42);
    Node* tree = build_mixed_tree(ar);
    Bytecode* bc = compileNode(ar, tree);
    VMResult res;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        res = runBytecode(db, bc);
    double elapsed = bench_time_ns() - start;
    (void)res;
    deleteFactDB(db);
    destroyArena(ar);
    return elapsed;
}

void bench_compare(BenchContext* ctx)
{
    double ns, elapsed;
    long mem;
    uint64_t it;

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_tree_walk_big, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "cmp_tree_walk_1023_nodes", "Compare",
                     "Tree-walk eval: 1023-node balanced AND tree",
                     ns, elapsed, it, mem, 1, 1, 0, 10);

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_bytecode_big, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "cmp_bytecode_1023_nodes", "Compare",
                     "Bytecode VM eval: same 1023-node tree",
                     ns, elapsed, it, mem, 1, 1, 0, 10);

    it = BENCH_ITER_MEDIUM;
    elapsed = bench_run(bench_tree_walk_mixed, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "cmp_tree_walk_mixed", "Compare",
                     "Tree-walk: mixed AND/NOT/OR/FACT/CMP (5 nodes)",
                     ns, elapsed, it, mem, 1, 2, 0, 5);

    it = BENCH_ITER_MEDIUM;
    elapsed = bench_run(bench_bytecode_mixed, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "cmp_bytecode_mixed", "Compare",
                     "Bytecode VM: same mixed condition (5 nodes)",
                     ns, elapsed, it, mem, 1, 2, 0, 5);
}
