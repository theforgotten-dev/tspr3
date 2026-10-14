param([string]$InstallPrefix = 'dist\tspr3')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $cmakeExe = Get-Tspr3CMake
    $ctestExe = Join-Path (Split-Path -Parent $cmakeExe) 'ctest.exe'
    & $cmakeExe -S . -B build\library -G 'Visual Studio 17 2022' -A x64
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de configuracion.' }
    & $cmakeExe --build build\library --config Release
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion.' }
    & $ctestExe --test-dir build\library -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de pruebas de biblioteca.' }
    & $cmakeExe --install build\library --config Release --prefix $InstallPrefix
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de instalacion local.' }
    & .\build\library\Release\tspr3_example_c.exe
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de ejemplo C.' }
    & .\build\library\Release\tspr3_example_cpp.exe
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de ejemplo C++.' }
    $resolvedPrefix = (Resolve-Path -LiteralPath $InstallPrefix).Path
    & $cmakeExe -S tests\consumer -B build\consumer -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$resolvedPrefix"
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de configuracion del consumidor instalado.' }
    & $cmakeExe --build build\consumer --config Release
    if ($LASTEXITCODE -ne 0) { throw 'Fallo de enlace del consumidor instalado.' }
    & .\build\consumer\Release\consumer_c.exe
    if ($LASTEXITCODE -ne 0) { throw 'Fallo del consumidor C instalado.' }
    & .\build\consumer\Release\consumer_cpp.exe
    if ($LASTEXITCODE -ne 0) { throw 'Fallo del consumidor C++ instalado.' }
    New-Item -ItemType Directory -Path dist -Force | Out-Null
    Compress-Archive -LiteralPath $resolvedPrefix -DestinationPath dist\tspr3-windows-x64-0.1.0.zip -Force
} finally { Pop-Location }
