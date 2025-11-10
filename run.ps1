# This script captures the whole workflow of running the Capylang compiler, converting
# the result WAT file to WASM and finally running the resulting WASI-compatible WASM
# module with a WASM runtime

param (
    [string]$InputFile = $(throw "Input file path is required")
)

# --- Compute relative paths manually (without requiring existence) ---
function Get-RelativePath($TargetPath, $BasePath) {
    $uriTarget = New-Object System.Uri($TargetPath)
    $uriBase   = New-Object System.Uri($BasePath + [System.IO.Path]::DirectorySeparatorChar)
    return $uriBase.MakeRelativeUri($uriTarget).ToString() -replace '/', [System.IO.Path]::DirectorySeparatorChar
}

# ------------------------------------------------------------------------------
$ResolvedInput = (Resolve-Path $InputFile).Path
$BaseName  = [System.IO.Path]::GetFileNameWithoutExtension($ResolvedInput)
$DirName   = [System.IO.Path]::GetDirectoryName($ResolvedInput)

$WatFile = Join-Path $DirName ($BaseName + ".wat")
$WasmFile = Join-Path $DirName ($BaseName + ".wasm")

# --- Build relative paths ---
$CurrentDir = (Resolve-Path ".").Path
$InputRel   = Get-RelativePath $ResolvedInput $CurrentDir
$WatFileRel = Get-RelativePath $WatFile $CurrentDir

$InputRel  = $InputRel -replace '\\', '/'
$WatFileRel = $WatFileRel -replace '\\', '/'

# Cleanup first
if (Test-Path $WatFile) {
    Remove-Item $WatFile
}
if (Test-Path $WasmFile) {
    Remove-Item $WasmFile
}

# Run the capylang compiler
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run --dir . .\build\capylang.wasm -i $InputRel -o $WatFileRel
if ($LASTEXITCODE -ne 0) {
    Write-Output "Compilation failed, terminating script"
    exit $LASTEXITCODE
}

# convert the WAT file to WASM
C:\sw\wasm-tools-1.230.0-x86_64-windows\wasm-tools.exe parse $WatFile -o $WasmFile

# Run the new WASM file with a WASM runtime
C:\sw\wasmtime-v33.0.0-x86_64-windows\wasmtime.exe run $WasmFile @args
Write-Output "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
