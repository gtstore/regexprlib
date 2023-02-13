cmake_minimum_required(VERSION 3.22)
project(test)

set(CMAKE_CXX_STANDARD 20)

include_directories(../regexprlib ../tree_graph)
set(SOURCES ../regexprlib/regexpr.cpp)
set(HEADERS ../regexprlib/regexpr.h ../tree_graph/textbox.h)
add_library(regexpr ${SOURCES} ${HEADERS})
add_executable(test test.cpp)
target_link_libraries(test regexpr)