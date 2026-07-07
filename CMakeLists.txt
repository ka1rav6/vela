cmake_minimum_required(VERSION 3.10)
project(rule_engine C)

find_library(YYJSON_LIB yyjson REQUIRED)

# Needed for pthread_mutex_t / pthread_rwlock_t used throughout the
# *_internal.h headers (Arena, FactDB, RuleEngine, Engine).
find_package(Threads REQUIRED)

# Valgrind for leak checking (used by multiple custom targets below)
find_program(VALGRIND_EXECUTABLE valgrind)

# ---------- common source files used by both executables ----------
set(CORE_SOURCES
    src/engine/factdb.c
    src/engine/rule.c
    src/engine/ConditionTree.c
    src/engine/parser_engine.c
    src/engine/semanticChecker.c
    src/engine/engine.c
    src/engine/ActionEntry.c
    src/engine/arena.c
    src/engine/bytecode.c
    src/engine/readBin.c
)

set(CORE_INCLUDES ${CMAKE_SOURCE_DIR}/include)
set(CORE_FLAGS -g -O0 -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L)

# ===================== Integration test (original) =====================
add_executable(engine ${CORE_SOURCES} test/testRunner.c)
target_include_directories(engine PRIVATE ${CORE_INCLUDES})
target_compile_options(engine PRIVATE ${CORE_FLAGS})
target_link_libraries(engine PRIVATE ${YYJSON_LIB} Threads::Threads)

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

# ===================== Unit tests (greatest.h) =====================
add_executable(unit_tests ${CORE_SOURCES} test/unit_tests.c)
target_include_directories(unit_tests PRIVATE ${CORE_INCLUDES})
target_compile_options(unit_tests PRIVATE ${CORE_FLAGS})
target_link_libraries(unit_tests PRIVATE ${YYJSON_LIB} Threads::Threads)

add_custom_target(
    run_unit_tests
    COMMAND $<TARGET_FILE:unit_tests>
    DEPENDS unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(VALGRIND_EXECUTABLE)
    add_custom_target(
        valgrind_unit_tests
        COMMAND ${VALGRIND_EXECUTABLE}
                --leak-check=full
                --show-leak-kinds=all
                --track-origins=yes
                --verbose
                --error-exitcode=1
                $<TARGET_FILE:unit_tests>
        DEPENDS unit_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
