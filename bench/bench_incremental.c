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

static BenchEnginePair build_incr_engine(int n_rules, int n_facts, int deps_per_rule)
{
    FactDB* db = createFactDB();
    RuleEngine* re = createRuleEngine();
    BenchEnginePair err = {NULL, NULL};
    if (!db || !re) return err;
    for (int i = 0; i < n_facts; i++) {
        char fname[64];
        snprintf(fname, 64, "bool_%d", i);
        setBoolFact(db, fname, true);
    }
    for (int ri = 0; ri < n_rules; ri++) {
        Node* cur = NULL;
        for (int di = 0; di < deps_per_rule; di++) {
            int fi = (ri * deps_per_rule + di) % n_facts;
            char fname[64];
            snprintf(fname, 64, "bool_%d", fi);
            Node* fact_node = createNode(re->arena, NODE_FACT);
            fact_node->data.Fact.factName = arena_strdup(re->arena, fname);
            if (!cur) {
                cur = fact_node;
            } else {
                Node* andN = createNode(re->arena, NODE_AND);
                andN->data.op.left = cur;
                andN->data.op.right = fact_node;
                cur = andN;
            }
        }
        if (!cur) {
            cur = createNode(re->arena, NODE_NULL);
        }
        char rname[64], aname[64];
        snprintf(rname, 64, "r_%d", ri);
        snprintf(aname, 64, "A_%d", ri % 10);
        Rule* r = createRule(re, cur, aname, rname, NULL);
        addRule(re, r);
    }
    BenchEnginePair ok = {db, re};
    return ok;
}

static double bench_cold_eval(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_incr_engine(500, 200, 3);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        rule_engine_mark_all_dirty((RuleEngine*)p.re);
        run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    }
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_warm_eval(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_incr_engine(500, 200, 3);
    run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++)
        run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_incr_eval(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_incr_engine(500, 200, 3);
    run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        setBoolFact((FactDB*)p.db, "bool_0", (i % 2) == 0);
        run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    }
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_sparse_incr(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_incr_engine(1000, 200, 2);
    run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        int fact_idx = (int)(i % 200);
        char fname[64];
        snprintf(fname, 64, "bool_%d", fact_idx);
        setBoolFact((FactDB*)p.db, fname, (i % 2) == 0);
        run_rules_silent((RuleEngine*)p.re, (FactDB*)p.db);
    }
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_mark_dirty(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_incr_engine(1000, 200, 3);
    double start = bench_time_ns();
    for (uint64_t i = 0; i < iters; i++) {
        int fact_idx = (int)(i % 200);
        char fname[64];
        snprintf(fname, 64, "bool_%d", fact_idx);
        rule_engine_mark_fact_dirty((RuleEngine*)p.re, fname);
    }
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

void bench_incremental(BenchContext* ctx)
{
    double ns, elapsed;
    long mem;
    uint64_t it;

    it = BENCH_ITER_SCARCE;
    elapsed = bench_run(bench_cold_eval, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "incr_cold_eval_500", "Incremental",
                     "Cold eval: 500 rules (all dirty) first run",
                     ns, elapsed, it, mem, 500, 200, 0, 3);

    it = BENCH_ITER_SCARCE;
    elapsed = bench_run(bench_warm_eval, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "incr_warm_eval_500", "Incremental",
                     "Warm eval: 500 rules (none dirty) skip cost",
                     ns, elapsed, it, mem, 500, 200, 0, 3);

    it = BENCH_ITER_SCARCE;
    elapsed = bench_run(bench_incr_eval, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "incr_after_change_500", "Incremental",
                     "Incremental: 500 rules after 1 fact change (~15 rules dirty)",
                     ns, elapsed, it, mem, 500, 200, 0, 3);

    it = BENCH_ITER_HEAVY;
    elapsed = bench_run(bench_sparse_incr, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "incr_sparse_1000", "Incremental",
                     "Sparse: 1000 rules, 200 facts, 2 deps/rule, change cycles",
                     ns, elapsed, it, mem, 1000, 200, 0, 2);

    it = BENCH_ITER_COMPLEX;
    elapsed = bench_run(bench_mark_dirty, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "incr_mark_dirty_1000", "Incremental",
                     "Dirty-marking: scan 1000 rule deps per fact change",
                     ns, elapsed, it, mem, 1000, 200, 0, 3);
}
