param(
    [string]$MsysRoot = 'C:\msys64',
    [string]$CMake = 'cmake',
    [string]$BuildDir = '',
    [int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (-not $BuildDir) { $BuildDir = Join-Path $RepoRoot 'build' }
$Ucrt = Join-Path $MsysRoot 'ucrt64'
$env:PATH = (Join-Path $Ucrt 'bin') + [IO.Path]::PathSeparator + $env:PATH
$ConfigureArgs = @('-S', $RepoRoot, '-B', $BuildDir, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DOPENGL_TYPE=OPENGL', "-DCMAKE_PREFIX_PATH=$($Ucrt.Replace('\','/'))", "-DCMAKE_C_COMPILER=$($Ucrt.Replace('\','/'))/bin/cc.exe", "-DCMAKE_CXX_COMPILER=$($Ucrt.Replace('\','/'))/bin/c++.exe", "-DCMAKE_MAKE_PROGRAM=$($Ucrt.Replace('\','/'))/bin/ninja.exe")
& $CMake @ConfigureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
& $CMake --build $BuildDir --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
$CompiledRows = Get-ChildItem -LiteralPath $BuildDir -Recurse -File | Where-Object { $_.Extension -in '.o', '.obj', '.exe' } | ForEach-Object {
    [pscustomobject]@{ File=$_.FullName; CompiledUtc=$_.LastWriteTimeUtc.ToString('o'); CompiledLocal=$_.LastWriteTime.ToString('o'); Bytes=$_.Length }
}
$CompiledRows | Export-Csv -LiteralPath (Join-Path $BuildDir 'compiled_timestamps.csv') -NoTypeInformation -Encoding UTF8
$ExePath = Join-Path $BuildDir 'avpt.exe'
Get-Item -LiteralPath $ExePath | Select-Object FullName,Length,LastWriteTime,LastWriteTimeUtc
Get-FileHash -LiteralPath $ExePath -Algorithm SHA256
Write-Output 'Local build complete. Nothing was pushed or released.'
