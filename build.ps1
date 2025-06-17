# C:\sw\wasi-sdk-25.0-x86_64-windows\bin\clang.exe .\hello.cpp --target=wasm32-wasip1 -fno-exceptions -lc++abi -lc++ --sysroot=C:\sw\wasi-sysroot-25.0\ -o hello.wasm

cd build
cmake --build .
cd ..
