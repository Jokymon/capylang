cmake_minimum_required(VERSION 3.18)
project(capylang C CXX)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/args_parse.cpp
    src/lexer.cpp
    src/emitter.cpp
    src/parser.cpp
    src/semantics.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
