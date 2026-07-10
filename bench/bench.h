#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BENCH_VERSION "1.0.0"

#define BENCH_MAX_RESULTS 256
#define BENCH_MAX_NAME 128
#define BENCH_MAX_DESC 256

#define BENCH_WARMUP 3
#define BENCH_ITER_SIMPLE 10000000ULL
#define BENCH_ITER_MEDIUM 1000000ULL
#define BENCH_ITER_COMPLEX 100000ULL
#define BENCH_ITER_SCARCE 10000ULL
#define BENCH_ITER_HEAVY 1000ULL

typedef struct {
    char name[BENCH_MAX_NAME];
    char category[BENCH_MAX_NAME];
    char desc[BENCH_MAX_DESC];
    double ns_per_op;
    double total_ns;
    uint64_t iterations;
    long mem_delta_kb;
    int n_rules;
    int n_facts;
    int n_threads;
    int depth;
    bool skipped;
    double min_ns;
    double max_ns;
} BenchResult;

typedef struct {
    BenchResult results[BENCH_MAX_RESULTS];
    int count;
} BenchContext;

typedef struct {
    void* db;
    void* re;
} BenchEnginePair;

static inline double bench_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static inline long bench_mem_kb(void)
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (long)usage.ru_maxrss;
}

void bench_add_result(BenchContext* ctx,
                      const char* name,
                      const char* category,
                      const char* desc,
                      double ns_per_op,
                      double total_ns,
                      uint64_t iterations,
                      long mem_delta_kb,
                      int n_rules,
                      int n_facts,
                      int n_threads,
                      int depth);

void bench_add_skip(BenchContext* ctx, const char* name, const char* category, const char* desc);

typedef double (*BenchFn)(uint64_t iterations, long* mem_delta);

double bench_run(BenchFn fn, uint64_t iterations, long* mem_delta);

int bench_parse_opts(int argc, char** argv, const char** include_filter, int* list_only);

void bench_vm(BenchContext* ctx);
void bench_engine(BenchContext* ctx);
void bench_incremental(BenchContext* ctx);
void bench_memory(BenchContext* ctx);
void bench_thread(BenchContext* ctx);
void bench_compare(BenchContext* ctx);

void bench_print_results(BenchContext* ctx);
void bench_export_json(BenchContext* ctx, const char* path);
