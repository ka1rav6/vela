cmake_minimum_required(VERSION 3.10)
project(rule_engine VERSION 1.0.0 LANGUAGES C)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ----- Dependencies -----
find_library(YYJSON_LIB yyjson REQUIRED)
find_package(Threads REQUIRED)
find_program(VALGRIND_EXECUTABLE valgrind)

# ----- Compiler flags -----
set(CORE_FLAGS -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L)
set(CORE_INCLUDES ${CMAKE_SOURCE_DIR}/include)

# ----- Core library (static) -----
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

add_library(vela STATIC ${CORE_SOURCES})
target_include_directories(vela PUBLIC ${CORE_INCLUDES})
target_compile_options(vela PRIVATE ${CORE_FLAGS})
target_link_libraries(vela PUBLIC ${YYJSON_LIB} Threads::Threads)

# ----- Demo / integration test runner -----
add_executable(engine test/testRunner.c)
target_link_libraries(engine PRIVATE vela)

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

# ----- Unit tests -----
add_executable(unit_tests test/unit_tests.c)
target_link_libraries(unit_tests PRIVATE vela)

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

# ----- Install -----
install(TARGETS vela
    ARCHIVE DESTINATION lib
)
install(TARGETS engine
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/
    DESTINATION include/vela
    FILES_MATCHING PATTERN "*.h"
)

# ----- CPack -----
set(CPACK_PACKAGE_NAME "vela")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Vela — A High-Performance Embeddable Rule Engine in C")
set(CPACK_PACKAGE_VENDOR "Vela")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/anomalyco/rule_engine")
set(CPACK_PACKAGE_CONTACT "https://github.com/anomalyco/rule_engine")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_GENERATOR "TGZ;DEB")
include(CPack)
