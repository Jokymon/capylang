cmake_minimum_required(VERSION 3.18)
project(capylang C CXX)

add_executable(${PROJECT_NAME}
    main.cpp
    args_parse.cpp
    emitter.cpp
    parser.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
