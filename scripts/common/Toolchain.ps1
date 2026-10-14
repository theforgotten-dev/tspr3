function Get-Tspr3VisualStudio {
    $locator=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if(!(Test-Path -LiteralPath $locator)) { throw 'Visual Studio locator not found.' }
    $installation=& $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if(!$installation) { throw 'MSVC C/C++ tools not found.' }
    return $installation
}
function Get-Tspr3CMake {
    $command=Get-Command cmake -ErrorAction SilentlyContinue
    if($command) { return $command.Source }
    $path=Join-Path (Get-Tspr3VisualStudio) 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if(!(Test-Path -LiteralPath $path)) { throw 'CMake not found.' }
    return $path
}
function Get-Tspr3ReleaseJdk {
    $candidates=@()
    if($env:JAVA_HOME) { $candidates+= $env:JAVA_HOME }
    $installed=Get-ChildItem -LiteralPath 'C:\Program Files\Eclipse Adoptium' -Directory -ErrorAction SilentlyContinue
    $candidates+=@($installed | Where-Object Name -Like 'jdk-21*' | ForEach-Object FullName)
    $candidates+=@($installed | ForEach-Object FullName)
    foreach($candidate in $candidates) {
        $release=Join-Path $candidate 'release'
        if(Test-Path -LiteralPath $release) {
            $text=Get-Content -LiteralPath $release -Raw
            if($text -match 'JAVA_VERSION="([0-9]+)') {
                if([int]$Matches[1] -ge 17) { return $candidate }
            }
        }
    }
    throw 'A JDK 17+ is required for Maven publication tooling.'
}
function Get-Tspr3JavaFlags([string]$JdkRoot) {
    $release=Get-Content -LiteralPath (Join-Path $JdkRoot 'release') -Raw
    if($release -match 'JAVA_VERSION="1\.8') { return @('-source','8','-target','8','-Xlint:all') }
    return @('--release','8','-Xlint:all,-options')
}
