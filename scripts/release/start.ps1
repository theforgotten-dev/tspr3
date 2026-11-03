param([switch]$EnablePublish)
$ErrorActionPreference='Stop'
if(!$env:TSPR3_WEBHOOK_SECRET) {
    $bytes=New-Object byte[] 32
    $rng=[Security.Cryptography.RandomNumberGenerator]::Create()
    try { $rng.GetBytes($bytes) } finally { $rng.Dispose() }
    $env:TSPR3_WEBHOOK_SECRET=[Convert]::ToBase64String($bytes)
}
$env:TSPR3_ALLOW_PUBLISH=if($EnablePublish) {'1'} else {'0'}
if($EnablePublish) {
    foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
        $secret=Read-Host $name -AsSecureString
        try { [Environment]::SetEnvironmentVariable($name,[Net.NetworkCredential]::new('',$secret).Password,'Process') }
        finally { $secret.Dispose() }
        if(![Environment]::GetEnvironmentVariable($name,'Process')) { throw 'Secret cannot be empty' }
    }
}
$state=Join-Path $env:LOCALAPPDATA 'TSPR3\pipeline'
if($env:TSPR3_PIPELINE_HOME) { $state=$env:TSPR3_PIPELINE_HOME }
[void][IO.Directory]::CreateDirectory($state)
try {
    $node=(Get-Command node -ErrorAction Stop).Source
    $script=Join-Path $PSScriptRoot 'pipeline.mjs'
    $process=Start-Process -FilePath $node -ArgumentList @('"'+$script+'"') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $state 'server.out.log') -RedirectStandardError (Join-Path $state 'server.err.log')
    Write-Host "Server PID: $($process.Id). Use request.mjs from this same terminal. Logs: $state"
} finally {
    foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
        Remove-Item "Env:$name" -ErrorAction SilentlyContinue
    }
}
