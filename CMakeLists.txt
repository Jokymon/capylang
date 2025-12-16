cmake_minimum_required(VERSION 3.18)
project(capylang C CXX)
include(CTest)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/args_parse.cpp
    src/ir.cpp
    src/lexer.cpp
    src/emitter.cpp
    src/parser.cpp
    src/semantics.cpp
    src/symbol.cpp
    src/wat_generator.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 26)

add_test (NAME compiler-tests
    COMMAND uv run pytest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)