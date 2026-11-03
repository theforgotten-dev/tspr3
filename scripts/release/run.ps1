param(
    [Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9-]{8,64}$')][string]$JobId,
    [ValidateSet('patch','minor','major')][string]$Bump='patch',
    [ValidateSet('check','publish')][string]$Mode='check',
    [Parameter(Mandatory)][string]$StateDirectory,
    [string]$SecretsFile
)
$ErrorActionPreference='Stop'
function Git-Run([string[]]$Arguments) {
    $result=& git @Arguments
    if($LASTEXITCODE -ne 0) { throw "Git failed: $($Arguments[0])" }
    return $result
}
function Save-State([hashtable]$Value) {
    $target=Join-Path $StateDirectory "$JobId.release.json";$temporary="$target.tmp"
    $Value.updated=(Get-Date).ToUniversalTime().ToString('o')
    $Value | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $temporary -Encoding UTF8
    Move-Item -LiteralPath $temporary -Destination $target -Force
}
function New-SignedTag([string]$Tag,[string]$Commit,[string]$Version,[string]$Gpg,[string]$Fingerprint) {
    $name=(Git-Run @('config','user.name')).Trim();$email=(Git-Run @('config','user.email')).Trim()
    if(!$name -or !$email) { throw 'Git user.name and user.email are required.' }
    $timestamp=[DateTimeOffset]::Now.ToUnixTimeSeconds();$offset=(Get-Date -Format zzz).Replace(':','')
    $body="object $Commit`ntype commit`ntag $Tag`ntagger $name <$email> $timestamp $offset`n`nTSPR3 $Version`n"
    $bodyPath=Join-Path $StateDirectory "$JobId.tag-body";$signaturePath="$bodyPath.asc";$objectPath="$bodyPath.object"
    [IO.File]::WriteAllText($bodyPath,$body,[Text.UTF8Encoding]::new($false))
    $env:MAVEN_GPG_PASSPHRASE | & $Gpg --batch --yes --armor --detach-sign --pinentry-mode loopback --passphrase-fd 0 --local-user $Fingerprint --output $signaturePath $bodyPath
    if($LASTEXITCODE -ne 0) { throw 'GPG tag signing failed.' }
    [IO.File]::WriteAllBytes($objectPath,[IO.File]::ReadAllBytes($bodyPath)+[IO.File]::ReadAllBytes($signaturePath))
    $tagObject=(Git-Run @('hash-object','-t','tag','-w',$objectPath)).Trim()
    Git-Run @('update-ref',"refs/tags/$Tag",$tagObject);Git-Run @('verify-tag',$Tag)
}
$remote='https://github.com/theforgotten-dev/tspr3.git';$checkout=Join-Path $StateDirectory $JobId
if(Test-Path -LiteralPath $checkout) { throw 'Job checkout exists. Inspect it; releases are never automatically retried.' }
Git-Run @('clone','--branch','main','--single-branch',$remote,$checkout)
Push-Location $checkout
$secretsLoaded=$false
try {
    $base=(Git-Run @('rev-parse','HEAD')).Trim();[xml]$pom=Get-Content java/pom.xml;$old=[string]$pom.project.version
    if($old -notmatch '^(\d+)\.(\d+)\.(\d+)$') { throw 'Expected a stable semantic version.' }
    $parts=@([int]$Matches[1],[int]$Matches[2],[int]$Matches[3])
    switch($Bump) { 'patch' {$parts[2]++} 'minor' {$parts[1]++;$parts[2]=0} 'major' {$parts[0]++;$parts[1]=0;$parts[2]=0} }
    $version=$parts -join '.';$tag="v$version"
    if(Git-Run @('ls-remote','--tags','origin',"refs/tags/$tag")) { throw 'Release tag already exists.' }
    $centralBase="https://repo.maven.apache.org/maven2/io/github/theforgotten-dev/tspr3/$version/tspr3-$version"
    try { [void](Invoke-WebRequest -UseBasicParsing -Uri "$centralBase.pom");throw 'Version already exists in Central.' }
    catch { if(!$_.Exception.Response -or [int]$_.Exception.Response.StatusCode -ne 404) { throw } }
    $files=@('pom.xml','java/pom.xml','CMakeLists.txt','include/tspr3/tspr3.h','java/src/main/java/org/tspr3/Tspr3.java','scripts/build-java.ps1','scripts/build-library.ps1')
    foreach($file in $files) {
        $text=[IO.File]::ReadAllText((Join-Path $checkout $file));if(!$text.Contains($old)) { throw "Version missing in $file" }
        [IO.File]::WriteAllText((Join-Path $checkout $file),$text.Replace($old,$version),[Text.UTF8Encoding]::new($false))
    }
    $date=Get-Date -Format 'yyyy-MM-dd';$changePath=Join-Path $checkout 'CHANGELOG.md';$changes=[IO.File]::ReadAllText($changePath)
    if(!$changes.Contains('## [Unreleased]')) { throw 'CHANGELOG.md needs an Unreleased section.' }
    [IO.File]::WriteAllText($changePath,$changes.Replace('## [Unreleased]',"## [Unreleased]`n`n## [$version] - $date"),[Text.UTF8Encoding]::new($false));$files+='CHANGELOG.md'
    Save-State @{job=$JobId;version=$version;base=$base;mode=$Mode;phase='validating'}
    & ./scripts/build-library.ps1;& ./scripts/build-java.ps1 -Native;& ./scripts/build-maven.ps1 -Goals @('clean','verify')
    if($Mode -eq 'check') { Save-State @{job=$JobId;version=$version;base=$base;mode=$Mode;phase='checked'};Write-Host "CHECK COMPLETE: $version. No push or Maven upload.";return }
    if($env:TSPR3_ALLOW_PUBLISH -ne '1') { throw 'Publication is disabled.' }
    if(!$SecretsFile) { throw 'SecretsFile is required for publication.' }
    . ./scripts/release/secrets.ps1;Import-Tspr3ReleaseSecrets -Path $SecretsFile;$secretsLoaded=$true
    & ./scripts/publish-central.ps1 -CheckOnly
    Git-Run (@('add','--')+$files);Git-Run @('commit','-m',"Release $version");$commit=(Git-Run @('rev-parse','HEAD')).Trim()
    $gpg='C:\Program Files\Git\usr\bin\gpg.exe';$fingerprint='3DA8BCEF0D9A7CADEE2390E1E7E1DFF927C45BD3'
    $keyHome=Join-Path $env:USERPROFILE '.tspr3-gnupg';$previousHome=$env:GNUPGHOME
    try {
        $env:GNUPGHOME='/'+$keyHome.Substring(0,1).ToLowerInvariant()+$keyHome.Substring(2).Replace('\','/')
        $deploymentOutput=& ./scripts/build-maven.ps1 -Goals @('-Pcentral-release',"-Dgpg.executable=$gpg","-Dgpg.keyname=$fingerprint",'-DautoPublish=false','-DwaitUntil=validated','deploy') 2>&1
        $deploymentOutput | ForEach-Object { Write-Host $_ };$deploymentText=$deploymentOutput -join "`n"
        if($deploymentText -notmatch 'deploymentId:\s*([0-9a-f-]{36})') { throw 'Central deployment ID was not found.' }
        $deploymentId=$Matches[1];Save-State @{job=$JobId;version=$version;base=$base;commit=$commit;tag=$tag;deploymentId=$deploymentId;phase='central-validated'}
        New-SignedTag -Tag $tag -Commit $commit -Version $version -Gpg $gpg -Fingerprint $fingerprint
    } finally { $env:GNUPGHOME=$previousHome }
    Git-Run @('push','--atomic','origin','HEAD:refs/heads/main',"refs/tags/$tag")
    Save-State @{job=$JobId;version=$version;base=$base;commit=$commit;tag=$tag;deploymentId=$deploymentId;phase='git-synchronized'}
    . ./scripts/release/central-api.ps1;Publish-Tspr3CentralDeployment -DeploymentId $deploymentId -TimeoutSeconds 1800
    Save-State @{job=$JobId;version=$version;base=$base;commit=$commit;tag=$tag;deploymentId=$deploymentId;phase='central-published'}
    $download=Join-Path $checkout "dist/tspr3-$version-central.jar";Invoke-WebRequest -UseBasicParsing -Uri "$centralBase.jar" -OutFile $download
    if((Get-FileHash $download -Algorithm SHA256).Hash -ne (Get-FileHash "java/target/tspr3-$version.jar" -Algorithm SHA256).Hash) { throw 'Central JAR hash mismatch.' }
    Save-State @{job=$JobId;version=$version;base=$base;commit=$commit;tag=$tag;deploymentId=$deploymentId;phase='published-and-verified'}
} finally { if($secretsLoaded) { Clear-Tspr3ReleaseSecrets };Pop-Location }
