# Wrapper script to run a WAT or WASM file with the WASI preview 1 API in the
# wasmtime runtime; in case of WAT files, it will first compile them to WASM
# using wasm-tools.exe

param (
    [string]$RunFile = $(throw "WAT or WASM file path is required")
)

$RunFile = (Resolve-Path $RunFile).Path

if ($RunFile.Extension -eq ".wat") {
    # convert the WAT file to WASM
    $WasmFile = [System.IO.Path]::ChangeExtension($RunFile, ".wasm")
    wasm-tools.exe parse $RunFile -o $WasmFile
    if ($LASTEXITCODE -ne 0) {
        Write-Output "WAT to WASM conversion failed, terminating script"
        exit $LASTEXITCODE
    }
    $RunFile = $WasmFile
}

wasmtime.exe run $RunFile @args
Write-Output "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
