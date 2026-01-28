set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSROOT C:/sw/wasi-sysroot-29.0)

set(target wasm32-wasip1)
set(tools C:/sw/wasi-sdk-29.0-x86_64-windows/bin)

set(CMAKE_C_COMPILER ${tools}/clang.exe)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER_TARGET ${target})

set(CMAKE_CXX_COMPILER ${tools}/clang.exe)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_TARGET ${target})
set(CMAKE_CXX_FLAGS -fno-exceptions)

set(CMAKE_EXECUTABLE_SUFFIX_C .wasm)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .wasm)

set(CMAKE_EXE_LINKER_FLAGS "-lc++abi -lc++")
