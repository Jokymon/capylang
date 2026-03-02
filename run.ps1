# This script captures the whole workflow of running the Capylang compiler, converting
# the result WAT file to WASM and finally running the resulting WASI-compatible WASM
# module with a WASM runtime

param (
    [string]$InputFile = $(throw "Input file path is required"),
    [switch]$Direct
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
$WasmFileRel = Get-RelativePath $WasmFile $CurrentDir

$InputRel  = $InputRel -replace '\\', '/'
$WatFileRel = $WatFileRel -replace '\\', '/'
$WasmFileRel = $WasmFileRel -replace '\\', '/'

# Cleanup first
if (Test-Path $WatFile) {
    Remove-Item $WatFile
}
if (Test-Path $WasmFile) {
    Remove-Item $WasmFile
}

# Run the capylang compiler
$CompilerOutput = if ($Direct) { $WasmFileRel } else { $WatFileRel }
wasmtime.exe run --dir . .\build\capylang.wasm -i $InputRel -o $CompilerOutput
if ($LASTEXITCODE -ne 0) {
    Write-Output "Compilation failed, terminating script"
    exit $LASTEXITCODE
}

if (-not $Direct) {
    # convert the WAT file to WASM
    wasm-tools.exe parse $WatFile -o $WasmFile
    if ($LASTEXITCODE -ne 0) {
        Write-Output "WAT to WASM conversion failed, terminating script"
        exit $LASTEXITCODE
    }
}

# Run the new WASM file with a WASM runtime
wasmtime.exe run $WasmFile @args
Write-Output "Lastexitcode: $LASTEXITCODE"
# Report the WASM runtime exit code to the outside
exit $LASTEXITCODE
