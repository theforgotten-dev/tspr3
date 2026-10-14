param([switch]$Native,[switch]$Benchmark,[switch]$AutoBenchmark,[switch]$Large)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $javacExe=(Get-Command javac -ErrorAction Stop).Source
    $jdkRoot=Split-Path -Parent (Split-Path -Parent $javacExe)
    $compilerFlags=Get-Tspr3JavaFlags $jdkRoot
    $javaExe=Join-Path $jdkRoot 'bin\java.exe';$jarExe=Join-Path $jdkRoot 'bin\jar.exe'
    $workspaceRoot=(Resolve-Path -LiteralPath '.').Path
    foreach($relative in @('build\java','build\java-tests','build\java-client')) {
        $target=[IO.Path]::GetFullPath((Join-Path $workspaceRoot $relative))
        if(!$target.StartsWith($workspaceRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid generated output path.' }
        if(Test-Path -LiteralPath $target) {
            if(((Get-Item -LiteralPath $target).Attributes -band [IO.FileAttributes]::ReparsePoint) -or
               ((Get-Item -LiteralPath (Join-Path $workspaceRoot 'build')).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Refusing to clean linked build directory.' }
            Remove-Item -LiteralPath $target -Recurse -Force
        }
    }
    New-Item -ItemType Directory -Path build\java,build\java-tests,build\java-client,dist,benchmarks\results -Force | Out-Null
    $mainFiles=@(Get-ChildItem -LiteralPath java\src\main\java -Recurse -Filter '*.java' | ForEach-Object FullName)
    & $javacExe -encoding UTF-8 @compilerFlags -Werror -d build\java @mainFiles
    if($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion Java.' }
    New-Item -ItemType Directory -Path build\java\META-INF -Force | Out-Null
    Copy-Item -LiteralPath LICENSE,NOTICE -Destination build\java\META-INF
    & $jarExe cf dist\tspr3-java-0.1.0.jar -C build\java .
    if($LASTEXITCODE -ne 0) { throw 'Fallo creando JAR.' }
    & $jarExe cf dist\tspr3-java-0.1.0-sources.jar -C java\src\main\java .
    & $jarExe uf dist\tspr3-java-0.1.0-sources.jar -C build\java META-INF
    if($LASTEXITCODE -ne 0) { throw 'Fallo incluyendo licencia en fuentes.' }
    $testFiles=@(Get-ChildItem -LiteralPath java\src\test\java -Recurse -Filter '*.java' | ForEach-Object FullName)
    & $javacExe -encoding UTF-8 @compilerFlags -Werror -cp dist\tspr3-java-0.1.0.jar -d build\java-tests @testFiles
    if($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion de pruebas Java.' }
    $testArguments=@()
    if($Large) { $testArguments+='--large' }
    & $javaExe -Xmx2g -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Tspr3Test @testArguments
    if($LASTEXITCODE -ne 0) { throw 'Fallo de pruebas Java.' }
    & $javacExe -cp dist\tspr3-java-0.1.0.jar -d build\java-client java\examples\UseTspr3.java
    if($LASTEXITCODE -ne 0) { throw 'Fallo de consumidor Java.' }
    & $javaExe -cp 'build\java-client;dist\tspr3-java-0.1.0.jar' UseTspr3
    if($LASTEXITCODE -ne 0) { throw 'Fallo de ejemplo Java.' }
    if($Native -or $Benchmark) {
        New-Item -ItemType Directory -Path dist\java-native -Force | Out-Null
        $installation=Get-Tspr3VisualStudio
        $devcmd=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
        @('@echo off',"call `"$devcmd`" -arch=x64 -host_arch=x64 >nul",'if errorlevel 1 exit /b 1',"cl /nologo /TC /std:c17 /Isrc\c\internal /Ibenchmarks\c /W4 /WX /O2 /MD /LD /Iinclude /I. /I`"$jdkRoot\include`" /I`"$jdkRoot\include\win32`" src\c\tspr3_tri.c src\c\tspr3_profile.c src\c\tspr3_log_selector.c src\c\tspr3_library.c java\native\tspr3_jni.c /Fo:build\ /Fe:dist\java-native\tspr3_jni.dll /link /IMPLIB:build\tspr3_jni.lib",'exit /b %errorlevel%') | Set-Content -LiteralPath build\compile_jni.cmd -Encoding ASCII
        & cmd.exe /d /c build\compile_jni.cmd
        if($LASTEXITCODE -ne 0) { throw 'Fallo de compilacion JNI.' }
        & $javaExe -Xcheck:jni -Xmx2g '-Djava.library.path=dist\java-native' -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Tspr3Test --native @testArguments
        if($LASTEXITCODE -ne 0) { throw 'Fallo de pruebas JNI.' }
    }
    if($Benchmark) {
        & $javaExe -Xms1g -Xmx2g '-Djava.library.path=dist\java-native' -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Benchmark
        if($LASTEXITCODE -ne 0) { throw 'Fallo del benchmark Java/JNI.' }
    }
    if($AutoBenchmark) {
        & $javaExe -Xms1g -Xmx2g -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Benchmark --auto-only
        if($LASTEXITCODE -ne 0) { throw 'Fallo de benchmark AUTO final.' }
    }
} finally { Pop-Location }
