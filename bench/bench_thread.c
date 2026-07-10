#include "bench.h"
#include "../include/rule.h"
#include "../include/rule_internal.h"
#include "../include/factdb.h"
#include "../include/arena.h"
#include "../include/ConditionTree.h"
#include "../include/bytecode.h"
#include <pthread.h>

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

typedef struct {
    RuleEngine* re;
    FactDB* db;
    uint64_t iterations;
    double elapsed_ns;
} ThreadWorkerArgs;

typedef struct {
    RuleEngine* re;
    FactDB* db;
    uint64_t iterations;
    double elapsed_ns;
} WriterWorkerArgs;

static void* worker_mark_and_run(void* arg)
{
    ThreadWorkerArgs* a = (ThreadWorkerArgs*)arg;
    for (uint64_t i = 0; i < a->iterations; i++) {
        rule_engine_mark_all_dirty(a->re);
        run_rules_silent(a->re, a->db);
    }
    return NULL;
}

static void* writer_worker(void* arg)
{
    WriterWorkerArgs* a = (WriterWorkerArgs*)arg;
    for (uint64_t i = 0; i < a->iterations; i++) {
        int fact_idx = (int)(i % 200);
        char fname[64];
        snprintf(fname, 64, "bool_%d", fact_idx);
        setBoolFact(a->db, fname, (i % 2) == 0);
        sched_yield();
    }
    return NULL;
}

static BenchEnginePair build_bench_engine(int n_rules, int n_facts)
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
        int f1 = ri % n_facts;
        int f2 = (ri + 1) % n_facts;
        char f1name[64], f2name[64];
        snprintf(f1name, 64, "bool_%d", f1);
        snprintf(f2name, 64, "bool_%d", f2);
        Node* b1 = createNode(re->arena, NODE_FACT);
        b1->data.Fact.factName = arena_strdup(re->arena, f1name);
        Node* b2 = createNode(re->arena, NODE_FACT);
        b2->data.Fact.factName = arena_strdup(re->arena, f2name);
        Node* andN = createNode(re->arena, NODE_AND);
        andN->data.op.left = b1;
        andN->data.op.right = b2;
        char rname[64], aname[64];
        snprintf(rname, 64, "r_%d", ri);
        snprintf(aname, 64, "A_%d", ri % 10);
        Rule* r = createRule(re, andN, aname, rname, NULL);
        addRule(re, r);
    }
    BenchEnginePair ok = {db, re};
    return ok;
}

static double bench_thread_2(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_bench_engine(500, 200);
    pthread_t t1, t2;
    ThreadWorkerArgs a1 = {(RuleEngine*)p.re, (FactDB*)p.db, iters / 2, 0};
    ThreadWorkerArgs a2 = {(RuleEngine*)p.re, (FactDB*)p.db, iters / 2, 0};
    pthread_create(&t1, NULL, worker_mark_and_run, &a1);
    pthread_create(&t2, NULL, worker_mark_and_run, &a2);
    double start = bench_time_ns();
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_thread_4(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_bench_engine(500, 200);
    pthread_t t[4];
    ThreadWorkerArgs args[4];
    for (int i = 0; i < 4; i++) {
        args[i] = (ThreadWorkerArgs){(RuleEngine*)p.re, (FactDB*)p.db, iters / 4, 0};
        pthread_create(&t[i], NULL, worker_mark_and_run, &args[i]);
    }
    double start = bench_time_ns();
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

static double bench_rw_contention(uint64_t iters, long* mem_delta)
{
    (void)mem_delta;
    BenchEnginePair p = build_bench_engine(500, 200);
    pthread_t writer_th, reader_th;
    WriterWorkerArgs wargs = {(RuleEngine*)p.re, (FactDB*)p.db, iters, 0};
    ThreadWorkerArgs rargs = {(RuleEngine*)p.re, (FactDB*)p.db, iters, 0};
    pthread_create(&writer_th, NULL, writer_worker, &wargs);
    pthread_create(&reader_th, NULL, worker_mark_and_run, &rargs);
    double start = bench_time_ns();
    pthread_join(writer_th, NULL);
    pthread_join(reader_th, NULL);
    double elapsed = bench_time_ns() - start;
    deleteRuleEngine((RuleEngine*)p.re);
    deleteFactDB((FactDB*)p.db);
    return elapsed;
}

void bench_thread(BenchContext* ctx)
{
    double ns, elapsed;
    long mem;
    uint64_t it;

    it = BENCH_ITER_HEAVY;
    elapsed = bench_run(bench_thread_2, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "thread_2_concurrent", "Thread",
                     "2 threads concurrent runEngine (500 rules, all dirty)",
                     ns, elapsed, it * 2, mem, 500, 200, 2, 2);

    it = BENCH_ITER_HEAVY / 2;
    elapsed = bench_run(bench_thread_4, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "thread_4_concurrent", "Thread",
                     "4 threads concurrent runEngine (500 rules, all dirty)",
                     ns, elapsed, it * 4, mem, 500, 200, 4, 2);

    it = BENCH_ITER_HEAVY;
    elapsed = bench_run(bench_rw_contention, it, &mem);
    ns = elapsed / it;
    bench_add_result(ctx, "thread_rw_contention", "Thread",
                     "1 writer + 1 reader: contention on FactDB rwlock",
                     ns, elapsed, it * 2, mem, 500, 200, 2, 2);
}
