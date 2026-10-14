param([switch]$Bench, [switch]$Million)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    New-Item -ItemType Directory -Path build -Force | Out-Null
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'No se encuentra vswhere; instala Visual Studio con herramientas C/C++.' }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$installation) { throw 'No se encuentra el compilador MSVC C/C++.' }
    $devcmd = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    # One cmd shell initializes MSVC and compiles; no filesystem deletion or moving.
    @("@echo off", "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", "if errorlevel 1 exit /b 1", 'cl /nologo /TC /std:c17 /Iexperiments\legacy\c /Ibenchmarks\c /W4 /WX /O2 experiments\legacy\c\tspr3.c experiments\legacy\c\tspr3_test.c /Fo:build\ /Fe:build\tspr3_test.exe', 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile.cmd -Encoding ASCII
    & cmd.exe /d /c build\compile.cmd
    if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación C.' }
    if ($Bench) { & .\build\tspr3_test.exe --bench } else { & .\build\tspr3_test.exe }
    if ($LASTEXITCODE -ne 0) { throw 'Falló la verificación.' }
    if ($Million) {
        New-Item -ItemType Directory -Path benchmarks\results -Force | Out-Null
        @('@echo off', "call `"$devcmd`" -arch=x64 -host_arch=x64 >nul", 'if errorlevel 1 exit /b 1', 'cl /nologo /TC /std:c17 /Iexperiments\legacy\c /Ibenchmarks\c /W4 /WX /O2 experiments\legacy\c\tspr3.c benchmarks\c\legacy_million.c /Fo:build\ /Fe:build\tspr3_million.exe', 'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_million.cmd -Encoding ASCII
        & cmd.exe /d /c build\compile_million.cmd
        if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación del benchmark de un millón.' }
        & .\build\tspr3_million.exe
        if ($LASTEXITCODE -ne 0) { throw 'Falló la verificación de un millón.' }
    }
} finally { Pop-Location }
