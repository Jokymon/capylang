C:\sw\wasm-tools-1.230.0-x86_64-windows\wasm-tools.exe parse example.wat -o example.wasm
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run .\example.wasm
exit $LASTEXITCODE
