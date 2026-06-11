cmake_minimum_required(VERSION 3.10)
project(rule_engine C)

find_library(YYJSON_LIB yyjson)

add_executable(engine 
    src/factdb.c 
    src/factdb.h 
    src/rule.c
    src/rule.h
    src/ConditionTree.h)

target_link_libraries(app ${YYJSON_LIB})
