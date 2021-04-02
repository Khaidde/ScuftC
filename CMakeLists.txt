cmake_minimum_required(VERSION 3.0.0)
set (CMAKE_CXX_STANDARD 17)

project(
  ScuftC
  VERSION 1.0
  LANGUAGES CXX)

include_directories(${PROJECT_SOURCE_DIR}/include)
set(SOURCES
    src/analysis.cpp
    src/ast_printer.cpp
    src/diagnostics.cpp
    src/lexer.cpp
    src/main.cpp
    src/parser.cpp
    src/sym_tab.cpp
)
add_executable(scft ${SOURCES})

# include_directories(${PROJECT_SOURCE_DIR}/include)
# add_executable(scft include/utest.cpp)