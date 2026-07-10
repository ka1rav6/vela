#include "bench.h"
#include <pthread.h>

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
                      int depth)
{
    if (ctx->count >= BENCH_MAX_RESULTS) return;
    BenchResult* r = &ctx->results[ctx->count++];
    snprintf(r->name, BENCH_MAX_NAME, "%s", name);
    snprintf(r->category, BENCH_MAX_NAME, "%s", category);
    snprintf(r->desc, BENCH_MAX_DESC, "%s", desc);
    r->ns_per_op = ns_per_op;
    r->total_ns = total_ns;
    r->iterations = iterations;
    r->mem_delta_kb = mem_delta_kb;
    r->n_rules = n_rules;
    r->n_facts = n_facts;
    r->n_threads = n_threads;
    r->depth = depth;
    r->skipped = false;
    r->min_ns = 0;
    r->max_ns = 0;
}

void bench_add_skip(BenchContext* ctx, const char* name, const char* category, const char* desc)
{
    if (ctx->count >= BENCH_MAX_RESULTS) return;
    BenchResult* r = &ctx->results[ctx->count++];
    snprintf(r->name, BENCH_MAX_NAME, "%s", name);
    snprintf(r->category, BENCH_MAX_NAME, "%s", category);
    snprintf(r->desc, BENCH_MAX_DESC, "%s", desc);
    r->skipped = true;
    r->ns_per_op = 0;
    r->total_ns = 0;
    r->iterations = 0;
    r->mem_delta_kb = 0;
    r->n_rules = 0;
    r->n_facts = 0;
    r->n_threads = 0;
    r->depth = 0;
}

double bench_run(BenchFn fn, uint64_t iterations, long* mem_delta)
{
    fflush(stdout);
    int saved_stdout = dup(STDOUT_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDOUT_FILENO);
    close(devnull);

    long mem_before = bench_mem_kb();
    for (int i = 0; i < BENCH_WARMUP; i++)
        fn(iterations / 100, NULL);
    if (mem_delta) *mem_delta = 0;
    double start = bench_time_ns();
    double result = fn(iterations, mem_delta);
    double elapsed = bench_time_ns() - start;
    if (mem_delta) {
        long mem_after = bench_mem_kb();
        *mem_delta = mem_after - mem_before;
    }

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    return result > 0 ? result : elapsed;
}

int bench_parse_opts(int argc, char** argv, const char** include_filter, int* list_only)
{
    *include_filter = NULL;
    *list_only = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0 || strcmp(argv[i], "-l") == 0) {
            *list_only = 1;
        } else if (strcmp(argv[i], "--bench") == 0 || strcmp(argv[i], "-b") == 0) {
            if (i + 1 < argc) *include_filter = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: bench [options]\n");
            printf("  -l, --list       List available benchmarks\n");
            printf("  -b, --bench PAT  Run only benchmarks matching PAT (substring match)\n");
            printf("  -h, --help       Show this help\n");
            return -1;
        }
    }
    return 0;
}

static int name_matches(const char* name, const char* filter)
{
    if (!filter || !*filter) return 1;
    return strstr(name, filter) != NULL;
}

static void print_separator(void)
{
    printf("+------+--------------------------------------+---------------------------+"
           "----------+----------+--------+--------+-------+\n");
}

static void print_header(void)
{
    print_separator();
    printf("| %-4s | %-36s | %-25s | %-8s | %-8s | %-6s | %-6s | %-5s |\n",
           " #", "Benchmark", "Category", "ns/op", "total", "iters", "rules", "memKB");
    print_separator();
}

void bench_print_results(BenchContext* ctx)
{
    printf("\n");
    print_header();
    for (int i = 0; i < ctx->count; i++) {
        BenchResult* r = &ctx->results[i];
        if (r->skipped) {
            printf("| %-4d | %-36s | %-25s | %-8s | %-8s | %-6s | %-6s | %-5s |\n",
                   i, r->name, r->category, "SKIP", "", "", "", "");
        } else {
            char ns_str[32], total_str[32];
            if (r->ns_per_op < 1) snprintf(ns_str, 32, "%.3f", r->ns_per_op);
            else if (r->ns_per_op < 1000) snprintf(ns_str, 32, "%.1f", r->ns_per_op);
            else snprintf(ns_str, 32, "%.0f", r->ns_per_op);
            double total_ms = r->total_ns / 1e6;
            if (total_ms < 1000) snprintf(total_str, 32, "%.1fms", total_ms);
            else snprintf(total_str, 32, "%.2fs", total_ms / 1000);
            printf("| %-4d | %-36s | %-25s | %-8s | %-8s | %-6llu | %-6d | %-5ld |\n",
                   i, r->name, r->category, ns_str, total_str,
                   (unsigned long long)r->iterations,
                   r->n_rules, r->mem_delta_kb);
        }
    }
    print_separator();
    printf("\n");
}

void bench_export_json(BenchContext* ctx, const char* path)
{
    FILE* f = stdout;
    if (path) {
        f = fopen(path, "w");
        if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"bench_version\": \"%s\",\n", BENCH_VERSION);
    fprintf(f, "  \"results\": [\n");
    for (int i = 0; i < ctx->count; i++) {
        BenchResult* r = &ctx->results[i];
        fprintf(f, "    {");
        fprintf(f, "\"name\":\"%s\",", r->name);
        fprintf(f, "\"category\":\"%s\",", r->category);
        fprintf(f, "\"desc\":\"%s\",", r->desc);
        fprintf(f, "\"ns_per_op\":%.3f,", r->ns_per_op);
        fprintf(f, "\"total_ns\":%.0f,", r->total_ns);
        fprintf(f, "\"iterations\":%llu,", (unsigned long long)r->iterations);
        fprintf(f, "\"skipped\":%s,", r->skipped ? "true" : "false");
        fprintf(f, "\"mem_delta_kb\":%ld,", r->mem_delta_kb);
        fprintf(f, "\"n_rules\":%d,", r->n_rules);
        fprintf(f, "\"n_facts\":%d,", r->n_facts);
        fprintf(f, "\"n_threads\":%d,", r->n_threads);
        fprintf(f, "\"depth\":%d", r->depth);
        fprintf(f, "}%s\n", (i < ctx->count - 1) ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    if (path) fclose(f);
}

int main(int argc, char** argv)
{
    const char* filter;
    int list_only;
    if (bench_parse_opts(argc, argv, &filter, &list_only) != 0)
        return 1;

    BenchContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    bench_vm(&ctx);
    bench_engine(&ctx);
    bench_incremental(&ctx);
    bench_memory(&ctx);
    bench_thread(&ctx);
    bench_compare(&ctx);

    if (filter) {
        for (int i = 0; i < ctx.count; i++) {
            if (!name_matches(ctx.results[i].name, filter) &&
                !name_matches(ctx.results[i].category, filter))
                ctx.results[i].skipped = true;
        }
    }

    if (list_only) {
        printf("Available benchmarks:\n");
        for (int i = 0; i < ctx.count; i++) {
            printf("  %-4d %-36s [%s]\n", i, ctx.results[i].name, ctx.results[i].category);
        }
        return 0;
    }

    bench_print_results(&ctx);
    bench_export_json(&ctx, NULL);

    char results_dir[256];
    snprintf(results_dir, 256, "%s/results", ".");
    mkdir(results_dir, 0755);
    char json_path[512];
    snprintf(json_path, 512, "%s/bench_results.json", results_dir);
    bench_export_json(&ctx, json_path);
    printf("Results saved to %s\n", json_path);

    return 0;
}
