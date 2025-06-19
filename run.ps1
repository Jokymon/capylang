# This script captures the whole workflow of running the Capylang compiler, converting
# the result WAT file to WASM and finally running the resulting WASI-compatible WASM
# module with a WASM runtime

# Cleanup first
if (Test-Path example.wat) {
    rm example.wat
}
if (Test-Path example.wasm) {
    rm example.wasm
}

# Run the capylang compiler
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run --dir . .\build\capylang.wasm -i example.capy -o example.wat

# convert the WAT file to WASM
C:\sw\wasm-tools-1.230.0-x86_64-windows\wasm-tools.exe parse example.wat -o example.wasm
# Run the new WASM file with a WASM runtime
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run .\example.wasm
echo "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
