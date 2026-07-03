cmake_minimum_required(VERSION 3.10)
project(rule_engine C)

find_library(YYJSON_LIB yyjson REQUIRED)

# Needed for pthread_mutex_t / pthread_rwlock_t used throughout the
# *_internal.h headers (Arena, FactDB, RuleEngine, Engine).
find_package(Threads REQUIRED)

add_executable(engine
    ## C files
    src/engine/factdb.c
    src/engine/rule.c
    src/engine/ConditionTree.c
    test/testRunner.c
    src/engine/parser_engine.c
    src/engine/semanticChecker.c
    src/engine/engine.c
    src/engine/ActionEntry.c
    src/engine/arena.c
    src/engine/bytecode.c
    src/engine/readBin.c
    # .h files
    include/uthash.h
    include/ConditionTree.h
    include/factdb.h
    include/factdb_internal.h
    include/engine.h
    include/engine_internal.h
    include/rule.h
    include/rule_internal.h
    include/semanticChecker.h
    include/ActionEntry.h
    include/ActionEntry_internal.h
    include/parser_engine.h
    include/arena.h
    include/arena_internal.h
    include/common.h
    include/bytecode.h
)

target_include_directories(engine PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Debug + gprof profiling
target_compile_options(engine PRIVATE
    -g
    -O0
    -pg
)

target_link_options(engine PRIVATE
    -pg
)

target_link_libraries(engine PRIVATE
    ${YYJSON_LIB}
    Threads::Threads
)

# Valgrind target
find_program(VALGRIND_EXECUTABLE valgrind)

if(VALGRIND_EXECUTABLE)
    add_custom_target(
        valgrind
        COMMAND ${VALGRIND_EXECUTABLE}
                --leak-check=full
                --show-leak-kinds=all
                --track-origins=yes
                --verbose
                $<TARGET_FILE:engine>
        DEPENDS engine
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
