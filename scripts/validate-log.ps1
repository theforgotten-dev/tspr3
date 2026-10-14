param([switch]$Tests,[switch]$Sanitize)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $installation=Get-Tspr3VisualStudio
    if (!$installation) { throw 'No se encuentra MSVC.' }
    $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    $mainSource = if ($Tests) { 'tests\c\profile_test.c' } else { 'benchmarks\c\log_validate.c' }
    $programName = if ($Tests) { 'tspr3_profile_test' } else { 'tspr3_log_validate' }
    $extraOptions = if ($Sanitize) { '/fsanitize=address /Zi' } else { '' }
    @('@echo off', "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', "cl /nologo /TC /std:c17 /Isrc\c\internal /Ibenchmarks\c /W4 /WX /O2 $extraOptions src\c\tspr3_tri.c src\c\tspr3_profile.c src\c\tspr3_log_selector.c $mainSource /Fo:build\ /Fe:build\$programName.exe", 'if errorlevel 1 exit /b 1', "build\$programName.exe", 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_log_validate.cmd -Encoding ASCII
    & cmd.exe /d /c build\compile_log_validate.cmd
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de validacion predictiva.' }
} finally { Pop-Location }
