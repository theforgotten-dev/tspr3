param([switch]$Million, [switch]$Sanitize)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    New-Item -ItemType Directory -Path build -Force | Out-Null
    $installation=Get-Tspr3VisualStudio
    if (!$installation) { throw 'No se encuentra MSVC C/C++.' }
    $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    $extraOptions = if ($Sanitize) { '/fsanitize=address /Zi' } else { '' }
    @('@echo off', "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', "cl /nologo /TC /std:c17 /Isrc\c\internal /Ibenchmarks\c /W4 /WX /O2 $extraOptions src\c\tspr3_tri.c tests\c\tri_test.c /Fo:build\ /Fe:build\tspr3_tri_test.exe", 'if errorlevel 1 exit /b 1', 'build\tspr3_tri_test.exe', 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_tri.cmd -Encoding ASCII
    & cmd.exe /d /c build\compile_tri.cmd
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion o verificacion de ternas.' }
    if ($Million) {
        New-Item -ItemType Directory -Path benchmarks\results -Force | Out-Null
        @('@echo off', "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', "cl /nologo /TC /std:c17 /Isrc\c\internal /Ibenchmarks\c /W4 /WX /O2 $extraOptions src\c\tspr3_tri.c benchmarks\c\tri_bench.c /Fo:build\ /Fe:build\tspr3_tri_bench.exe", 'if errorlevel 1 exit /b 1', 'build\tspr3_tri_bench.exe', 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_tri_bench.cmd -Encoding ASCII
        & cmd.exe /d /c build\compile_tri_bench.cmd
        if ($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion o benchmark de ternas.' }
    }
} finally { Pop-Location }
