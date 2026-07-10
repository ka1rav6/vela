add_executable(bench
    bench.c
    bench_vm.c
    bench_engine.c
    bench_incremental.c
    bench_memory.c
    bench_thread.c
    bench_compare.c
)
target_link_libraries(bench PRIVATE vela)
target_compile_options(bench PRIVATE -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -DNDEBUG)
