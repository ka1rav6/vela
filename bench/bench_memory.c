#include "bench.h"
#include "../include/rule.h"
#include "../include/rule_internal.h"
#include "../include/factdb.h"
#include "../include/arena.h"
#include "../include/arena_internal.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"

static double bench_arena_efficiency(uint64_t iters, long* mem_delta)
{
    (void)iters;
    (void)mem_delta;
    Arena* ar = createArena(1024 * 1024);
    double start = bench_time_ns();
    for (int i = 0; i < 10000; i++) {
        int* p = (int*)arena_alloc(ar, sizeof(int));
        if (p) *p = i;
    }
    double elapsed = bench_time_ns() - start;
    destroyArena(ar);
    return elapsed;
}

static double bench_per_rule_overhead(uint64_t iters, long* mem_delta)
{
    (void)iters;
    FactDB* db = createFactDB();
    RuleEngine* re = createRuleEngine();
    long mem_before = bench_mem_kb();
    double start = bench_time_ns();
    for (int i = 0; i < 1000; i++) {
        char fname[64], rname[64], aname[64];
        snprintf(fname, 64, "f_%d", i);
        snprintf(rname, 64, "r_%d", i);
        snprintf(aname, 64, "A_%d", i % 10);
        setBoolFact(db, fname, true);
        Node* n = createNode(re->arena, NODE_FACT);
        n->data.Fact.factName = arena_strdup(re->arena, fname);
        Rule* r = createRule(re, n, aname, rname, NULL);
        addRule(re, r);
    }
    double elapsed = bench_time_ns() - start;
    long mem_after = bench_mem_kb();
    deleteRuleEngine(re);
    deleteFactDB(db);
    if (mem_delta) *mem_delta = (mem_after - mem_before) * 1024;
    return elapsed;
}

static double bench_arena_vs_malloc(uint64_t iters, long* mem_delta)
{
    (void)iters;
    Arena* ar = createArena(1024 * 1024);
    long mem_before = bench_mem_kb();
    double start = bench_time_ns();
    char* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = (char*)arena_alloc(ar, 64);
        if (ptrs[i]) memset(ptrs[i], i, 64);
    }
    double elapsed = bench_time_ns() - start;
    long mem_arena = bench_mem_kb() - mem_before;
    destroyArena(ar);

    mem_before = bench_mem_kb();
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = (char*)malloc(64);
        if (ptrs[i]) memset(ptrs[i], i, 64);
    }
    long mem_malloc = bench_mem_kb() - mem_before;
    for (int i = 0; i < 1000; i++) free(ptrs[i]);

    if (mem_delta) *mem_delta = (mem_malloc - mem_arena) * 1024;
    return elapsed;
}

void bench_memory(BenchContext* ctx)
{
    double ns, elapsed;
    long mem;

    elapsed = bench_run(bench_arena_efficiency, 1, &mem);
    ns = elapsed;
    bench_add_result(ctx, "mem_arena_efficiency", "Memory",
                     "Arena bump allocator utilization", ns, elapsed, 1, mem, 10000, 0, 0, 0);

    elapsed = bench_run(bench_per_rule_overhead, 1, &mem);
    ns = elapsed;
    bench_add_result(ctx, "mem_per_rule_bytes", "Memory",
                     "Total bytes for 1000 simple bool rules", ns, elapsed, 1, mem, 1000, 1000, 0, 0);

    elapsed = bench_run(bench_arena_vs_malloc, 1, &mem);
    ns = elapsed;
    bench_add_result(ctx, "mem_arena_vs_malloc_savings", "Memory",
                     "Total bytes saved using arena vs malloc (1000 allocs)", ns, elapsed, 1, mem, 0, 0, 0, 0);
}
