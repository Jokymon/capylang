# Wrapper script for the capylang compiler
# The script expects one mandatory argument which must be the capylang source
# file to compile.
# Additionally the following arguments are available:

$inputFile = $null
$outputFile = $null
$forwardArgs = @()

# --- Compute relative paths manually (without requiring existence) ---
function Get-RelativePath($TargetPath) {
    $ResolvedInput = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($TargetPath)
    $CurrentDir = (Resolve-Path ".").Path
    
    $uriTarget = New-Object System.Uri($ResolvedInput)
    $uriBase   = New-Object System.Uri($CurrentDir + [System.IO.Path]::DirectorySeparatorChar)
    return $uriBase.MakeRelativeUri($uriTarget).ToString() -replace '/', [System.IO.Path]::DirectorySeparatorChar
}

# --- process input and output paths for running them in WASM runtime ---
for ($i = 0; $i -lt $args.Count; $i++) {
    switch ($args[$i]) {
        '-i' {
            $inputFile = Get-RelativePath $args[++$i]

            $forwardArgs += '-i'
            $forwardArgs += $inputFile
        }

        '-o' {
            $outputFile = Get-RelativePath $args[++$i]
            $forwardArgs += '-o'
            $forwardArgs += $outputFile
        }

        default {
            $forwardArgs += $args[$i]
        }
    }
}

wasmtime.exe run --dir . .\build\capylang.wasm @forwardArgs
exit $LASTEXITCODE
