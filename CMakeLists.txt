cmake_minimum_required(VERSION 3.18)
project(capylang C CXX)
include(CTest)
include(FetchContent)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/anf.cpp
    src/args_parse.cpp
    src/ast.cpp
    src/diagnostics.cpp
    src/normalization.cpp
    src/lexer.cpp
    src/lower_cabi.cpp
    src/emitter.cpp
    src/parser.cpp
    src/semantics.cpp
    src/symbol.cpp
    src/type_inference.cpp
    src/wasm_generator.cpp
    src/wat_generator.cpp
    src/wasm_ir.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 26)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)

# Since we are compiling for WASM target, let's disable some unsupported features
# in Catch2 such as signals and Windows SEH support
set(CATCH_CONFIG_NO_POSIX_SIGNALS ON CACHE BOOL "Disable Catch2 POSIX signal handling" FORCE)
set(CATCH_CONFIG_NO_WINDOWS_SEH ON CACHE BOOL "Disable Catch2 Windows SEH handling" FORCE)
FetchContent_MakeAvailable(Catch2)

add_executable(capylang-unit-tests
    src/tests/test_argparser.cpp
    src/args_parse.cpp
    ${Catch2_SOURCE_DIR}/src/catch2/internal/catch_main.cpp
)

set_property(TARGET capylang-unit-tests PROPERTY CXX_STANDARD 26)
target_link_libraries(capylang-unit-tests PRIVATE Catch2::Catch2)

add_test(
    NAME capylang-unit-tests
    COMMAND capylang-unit-tests
)

add_test (NAME compiler-tests
    COMMAND uv run pytest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
