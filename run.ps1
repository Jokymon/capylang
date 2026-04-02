# Wrapper script to run a WASM file with the WASI preview 1 API in the
# wasmtime runtime

param (
    [string]$WasmFile = $(throw "WASM file path is required")
)

wasmtime.exe run $WasmFile @args
Write-Output "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
