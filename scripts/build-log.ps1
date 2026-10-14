$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    New-Item -ItemType Directory -Path build -Force | Out-Null
    New-Item -ItemType Directory -Path benchmarks\results -Force | Out-Null
    $installation=Get-Tspr3VisualStudio
    if (!$installation) { throw 'No se encuentra MSVC.' }
    $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    @('@echo off', "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', 'cl /nologo /TC /std:c17 /Isrc\c\internal /Ibenchmarks\c /W4 /WX /O2 src\c\tspr3_tri.c src\c\tspr3_profile.c benchmarks\c\log_bench.c /Fo:build\ /Fe:build\tspr3_log_bench.exe', 'if errorlevel 1 exit /b 1', 'build\tspr3_log_bench.exe', 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_log.cmd -Encoding ASCII
    & cmd.exe /d /c build\compile_log.cmd
    if ($LASTEXITCODE -ne 0) { throw 'Fallo en experimento logaritmico.' }
} finally { Pop-Location }
