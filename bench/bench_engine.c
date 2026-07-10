#include "bench.h"
#include "../include/rule.h"
#include "../include/rule_internal.h"
#include "../include/factdb.h"
#include "../include/arena.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"

static void run_rules_silent(RuleEngine* re, FactDB* db)
{
    pthread_mutex_lock(&re->lock);
    Rule *cr, *tmp;
    HASH_ITER(hh, re->rules, cr, tmp)
    {
        if (!cr->dirty) continue;
        VMResult result = runBytecode(db, cr->bc);
        cr->dirty = false;
        (void)result;
    }
    pthread_mutex_unlock(&re->lock);
}

static Node* gen_and_compare_rule(Arena* ar, FactDB* db, int idx)
{
    char bname[64], nname[64];
    snprintf(bname, 64, "bf_%d", idx);
    snprintf(nname, 64, "nf_%d", idx);
    setBoolFact(db, bname, true);
    setNumFact(db, nname, (double)(idx * 10));

    Node* b = createNode(ar, NODE_FACT);
    b->data.Fact.factName = arena_strdup(ar, bname);
    Node* c = createNode(ar, NODE_COMPARE);
    c->data.Compare.factName = arena_strdup(ar, nname);
    c->data.Compare.op = OP_GT;
    c->data.Compare.val = (idx % 100) * 10;
    Node* a = createNode(ar, NODE_AND);
    a->data.op.left = b;
    a->data.op.right = c;
    return a;
}

static RuleEngine* build_engine(FactDB* db, int n_rules)
{
    RuleEngine* re = createRuleEngine();
    if (!re) return NULL;
    for (int i = 0; i < n_rules; i++) {
        Node* cond = gen_and_compare_rule(re->arena, db, i);
        char rname[64], aname[64];
        snprintf(rname, 64, "rule_%d", i);
        snprintf(aname, 64, "ACT_%d", i % 20);
        Rule* r = createRule(re, cond, aname, rname, NULL);
        if (!r) { deleteRuleEngine(re); return NULL; }
        addRule(re, r);
    }
    return re;
}

static double bench_init_100(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        FactDB* db = createFactDB();
        RuleEngine* re = build_engine(db, 100);
        deleteRuleEngine(re);
        deleteFactDB(db);
    }
    return bench_time_ns() - start;
}

static double bench_run_rules(uint64_t iters, long* mem_delta, int n_rules)
{
    (void)mem_delta;
    FactDB* db = createFactDB();
    RuleEngine* re = build_engine(db, n_rules);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        rule_engine_mark_all_dirty(re);
        run_rules_silent(re, db);
    }
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine(re);
    deleteFactDB(db);
    return elapsed;
}

static double bench_run_100(uint64_t iters, long* md) { return bench_run_rules(iters, md, 100); }
static double bench_run_500(uint64_t iters, long* md) { return bench_run_rules(iters, md, 500); }
static double bench_run_1000(uint64_t iters, long* md) { return bench_run_rules(iters, md, 1000); }

static double bench_factdb_bool_lookup(uint64_t iters, long* mem_delta)
{
    FactDB* db = createFactDB();
    for (int i = 0; i < 200; i++) {
        char name[64];
        snprintf(name, 64, "fact_%d", i);
        setBoolFact(db, name, (i % 2) == 0);
    }
    long mem_before = bench_mem_kb();
    bool sum = false;
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        char name[64];
        snprintf(name, 64, "fact_%d", (int)(i % 200));
        sum ^= getBoolFact(db, name);
    }
    double elapsed = bench_time_ns() - start;
    if (mem_delta) {
        long mem_after = bench_mem_kb();
        *mem_delta = mem_after - mem_before;
    }
    (void)sum;
    deleteFactDB(db);
    return elapsed;
}

void bench_engine(BenchContext* ctx)
{
    double ns, elapsed;
    long mem;
    uint64_t it;

    it = BENCH_ITER_HEAVY;
    elapsed = bench_run(bench_init_100, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "engine_init_100_rules", "Engine",
                     "Create engine with 100 rules", ns, elapsed, it, mem, 100, 100, 0, 0);

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_run_100, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "engine_run_100_rules", "Engine",
                     "Run 100 rules (all dirty, AND+CMP)", ns, elapsed, it, mem, 100, 200, 0, 2);

    it = BENCH_ITER_SCARCE;
    elapsed = bench_run(bench_run_500, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "engine_run_500_rules", "Engine",
                     "Run 500 rules (all dirty, AND+CMP)", ns, elapsed, it, mem, 500, 1000, 0, 2);

    it = BENCH_ITER_HEAVY;
    elapsed = bench_run(bench_run_1000, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "engine_run_1000_rules", "Engine",
                     "Run 1000 rules (all dirty, AND+CMP)", ns, elapsed, it, mem, 1000, 2000, 0, 2);

    it = BENCH_ITER_SIMPLE;
    elapsed = bench_run(bench_factdb_bool_lookup, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "factdb_bool_lookup", "Engine",
                     "Bool fact lookup in 200-entry hash table", ns, elapsed, it, mem, 0, 200, 0, 0);
}
