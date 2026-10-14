param([string[]]$Goals=@('verify'))
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'common\Toolchain.ps1')
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $version='3.9.16'
    $mavenRoot=Join-Path (Resolve-Path -LiteralPath '.').Path ".tools\apache-maven-$version"
    $mavenExe=Join-Path $mavenRoot 'bin\mvn.cmd'
    if(!(Test-Path -LiteralPath $mavenExe)) {
        New-Item -ItemType Directory -Path .tools -Force | Out-Null
        $archive=".tools\apache-maven-$version-bin.zip"
        $url="https://repo.maven.apache.org/maven2/org/apache/maven/apache-maven/$version/apache-maven-$version-bin.zip"
        Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $archive
        $expected=(Invoke-WebRequest -UseBasicParsing -Uri "$url.sha512").Content.Trim().Split(' ')[0].ToLowerInvariant()
        $actual=(Get-FileHash -LiteralPath $archive -Algorithm SHA512).Hash.ToLowerInvariant()
        if($actual -ne $expected) { throw 'Maven distribution checksum mismatch.' }
        Expand-Archive -LiteralPath $archive -DestinationPath .tools -Force
    }
    $jdk=Get-Tspr3ReleaseJdk
    $previousJavaHome=$env:JAVA_HOME
    try {
        $env:JAVA_HOME=$jdk
        & $mavenExe --batch-mode --no-transfer-progress --settings config\maven\settings.example.xml '-Dmaven.repo.local=.tools/m2' -f java\pom.xml @Goals
        if($LASTEXITCODE -ne 0) { throw 'Maven build failed.' }
    } finally { $env:JAVA_HOME=$previousJavaHome }
} finally { Pop-Location }
