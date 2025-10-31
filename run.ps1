# This script captures the whole workflow of running the Capylang compiler, converting
# the result WAT file to WASM and finally running the resulting WASI-compatible WASM
# module with a WASM runtime

param (
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$InputFile
)

# ------------------------------------------------------------------------------
$InputFile = (Resolve-Path $InputFile).Path
$BaseName  = [System.IO.Path]::GetFileNameWithoutExtension($InputFile)
$DirName   = [System.IO.Path]::GetDirectoryName($InputFile)

$WatFile = Join-Path $DirName ($BaseName + ".wat")
$WasmFile = Join-Path $DirName ($BaseName + ".wasm")

# --- Build relative paths (relative to $DirName) ---
Push-Location $DirName
try {
    $InputRel  = [System.IO.Path]::GetFileName($InputFile)
    $WatRel = [System.IO.Path]::GetFileName($WatFile)
} finally {
    Pop-Location
}

# Cleanup first
if (Test-Path $WatFile) {
    Remove-Item $WatFile
}
if (Test-Path $WasmFile) {
    Remove-Item $WasmFile
}

# Run the capylang compiler
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run --dir . .\build\capylang.wasm -i $InputRel -o $WatRel
if ($LASTEXITCODE -ne 0) {
    Write-Output "Compilation failed, terminating script"
    exit $LASTEXITCODE
}

# convert the WAT file to WASM
C:\sw\wasm-tools-1.230.0-x86_64-windows\wasm-tools.exe parse $WatFile -o $WasmFile

# Run the new WASM file with a WASM runtime
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run $WasmFile
Write-Output "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
