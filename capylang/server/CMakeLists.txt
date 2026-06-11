cmake_minimum_required(VERSION 3.18)
project(capylsp C CXX)
include(FetchContent)

FetchContent_Declare(
    lsp
    GIT_REPOSITORY https://github.com/Jokymon/lsp-framework-noec.git
    GIT_TAG v2.0.0
)
set(LSP_BUILD_HOST_TOOLS OFF CACHE BOOL "Use the WASM build for lspgen" FORCE)
FetchContent_MakeAvailable(lsp)

add_executable(${PROJECT_NAME}
    src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE lsp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 26)
