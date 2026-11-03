param(
    [Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9-]{8,64}$')][string]$JobId,
    [ValidateSet('patch','minor','major')][string]$Bump='patch',
    [ValidateSet('check','publish')][string]$Mode='check',
    [Parameter(Mandatory)][string]$StateDirectory
)
$ErrorActionPreference='Stop'
function Git-Run([string[]]$Arguments) {
    $result = & git @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Git failed: $($Arguments[0])" }
    return $result
}
$remote='https://github.com/theforgotten-dev/tspr3.git'
$checkout=Join-Path $StateDirectory $JobId
if(Test-Path -LiteralPath $checkout) { throw 'Job checkout exists. Inspect it; releases are never automatically retried.' }
Git-Run @('clone','--branch','main','--single-branch',$remote,$checkout)
Push-Location $checkout
try {
    $base=(Git-Run @('rev-parse','HEAD')).Trim()
    [xml]$pom=Get-Content java/pom.xml
    $old=[string]$pom.project.version
    if($old -notmatch '^(\d+)\.(\d+)\.(\d+)$') { throw 'Expected a stable semantic version.' }
    $parts=@([int]$Matches[1],[int]$Matches[2],[int]$Matches[3])
    switch($Bump) { 'patch' {$parts[2]++} 'minor' {$parts[1]++;$parts[2]=0} 'major' {$parts[0]++;$parts[1]=0;$parts[2]=0} }
    $version=$parts -join '.'
    $tag="v$version"
    if(Git-Run @('ls-remote','--tags','origin',"refs/tags/$tag")) { throw 'Release tag already exists.' }
    $url="https://repo.maven.apache.org/maven2/io/github/theforgotten-dev/tspr3/$version/tspr3-$version.pom"
    try { [void](Invoke-WebRequest -UseBasicParsing -Uri $url); throw 'Version already exists in Central.' }
    catch { if(!$_.Exception.Response -or [int]$_.Exception.Response.StatusCode -ne 404) { throw } }
    $files=@('pom.xml','java/pom.xml','CMakeLists.txt','include/tspr3/tspr3.h','java/src/main/java/org/tspr3/Tspr3.java','scripts/build-java.ps1','scripts/build-library.ps1')
    foreach($file in $files) {
        $text=[IO.File]::ReadAllText((Join-Path $checkout $file))
        if(!$text.Contains($old)) { throw "Version missing in $file" }
        [IO.File]::WriteAllText((Join-Path $checkout $file),$text.Replace($old,$version),[Text.UTF8Encoding]::new($false))
    }
    # Also support the existing remote baseline, where autoPublish was a literal false.
    $pomPath=Join-Path $checkout 'java/pom.xml'
    $pomText=[IO.File]::ReadAllText($pomPath)
    if($pomText.Contains('<autoPublish>false</autoPublish></configuration>')) {
        $pomText=$pomText.Replace('<properties>','<properties><autoPublish>false</autoPublish><waitUntil>validated</waitUntil>')
        $pomText=$pomText.Replace('<autoPublish>false</autoPublish></configuration>','<autoPublish>${autoPublish}</autoPublish><waitUntil>${waitUntil}</waitUntil></configuration>')
        [IO.File]::WriteAllText($pomPath,$pomText,[Text.UTF8Encoding]::new($false))
    }
    if($Mode -eq 'publish') { & ./scripts/publish-central.ps1 -CheckOnly }
    @{version=$version;base=$base;mode=$Mode;phase='validating'} | ConvertTo-Json | Set-Content (Join-Path $StateDirectory "$JobId.release.json")
    & ./scripts/build-library.ps1
    & ./scripts/build-java.ps1 -Native
    & ./scripts/build-maven.ps1 -Goals @('clean','verify')
    if($Mode -eq 'check') { Write-Host "CHECK COMPLETE: $version. No push or Maven upload."; return }
    foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
        if(![Environment]::GetEnvironmentVariable($name,'Process')) { throw "Missing environment variable $name" }
    }
    if($env:TSPR3_ALLOW_PUBLISH -ne '1') { throw 'Publication is disabled.' }
    Git-Run (@('add','--')+$files)
    Git-Run @('commit','-m',"Release $version")
    $commit=(Git-Run @('rev-parse','HEAD')).Trim()
    Git-Run @('tag','-a',$tag,'-m',"TSPR3 $version")
    # Fast-forward + atomic push: a concurrent main update rejects both refs.
    Git-Run @('push','--atomic','origin','HEAD:refs/heads/main',"refs/tags/$tag")
    @{version=$version;base=$base;commit=$commit;tag=$tag;phase='git-synchronized'} | ConvertTo-Json | Set-Content (Join-Path $StateDirectory "$JobId.release.json")
    $gpg='C:\Program Files\Git\usr\bin\gpg.exe'
    $keyHome=Join-Path $env:USERPROFILE '.tspr3-gnupg'
    $previousHome=$env:GNUPGHOME
    try {
        $env:GNUPGHOME='/'+$keyHome.Substring(0,1).ToLowerInvariant()+$keyHome.Substring(2).Replace('\','/')
        & ./scripts/build-maven.ps1 -Goals @('-Pcentral-release',"-Dgpg.executable=$gpg",'-Dgpg.keyname=3DA8BCEF0D9A7CADEE2390E1E7E1DFF927C45BD3','-DautoPublish=true','-DwaitUntil=published','deploy')
    } finally { $env:GNUPGHOME=$previousHome }
    $download=Join-Path $checkout "dist/tspr3-$version-central.jar"
    Invoke-WebRequest -UseBasicParsing -Uri $url.Replace('.pom','.jar') -OutFile $download
    if((Get-FileHash $download).Hash -ne (Get-FileHash "java/target/tspr3-$version.jar").Hash) { throw 'Central JAR hash mismatch.' }
    @{version=$version;base=$base;commit=$commit;tag=$tag;phase='published-and-verified'} | ConvertTo-Json | Set-Content (Join-Path $StateDirectory "$JobId.release.json")
} finally { Pop-Location }
