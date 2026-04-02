# Wrapper script for the capylang compiler
# The script expects one mandatory argument which must be the capylang source
# file to compile.
# Additionally the following arguments are available:
#
# -Direct       Normally the script will compile the source file to a WAT file
#               and then run wasm-tools to turn this into a WASM file. Using
#               this option, the compiler will use its own WASM generator
# -DumpAst      When this option is present, the compiler will only print the
#               AST on stdout without generating any output files.

param (
    [string]$InputFile = $(throw "Input file path is required"),
    [switch]$Direct,
    [switch]$DumpAst
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

$AdditionalArgs = ""
if ($DumpAst) {
    $AdditionalArgs = "--dump-ast"
}

# Run the capylang compiler
$CompilerOutput = if ($Direct) { $WasmFileRel } else { $WatFileRel }
wasmtime.exe run --dir . .\build\capylang.wasm $AdditionalArgs -i $InputRel -o $CompilerOutput
if ($LASTEXITCODE -ne 0) {
    Write-Output "Compilation failed, terminating script"
    exit $LASTEXITCODE
}

if (-not $Direct -and -not $DumpAst) {
    # convert the WAT file to WASM
    wasm-tools.exe parse $WatFile -o $WasmFile
    if ($LASTEXITCODE -ne 0) {
        Write-Output "WAT to WASM conversion failed, terminating script"
        exit $LASTEXITCODE
    }
}

exit $LASTEXITCODE
