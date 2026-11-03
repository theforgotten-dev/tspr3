param([switch]$EnablePublish)
$ErrorActionPreference='Stop'
if(!$env:TSPR3_WEBHOOK_SECRET) {
    $bytes=New-Object byte[] 32;$rng=[Security.Cryptography.RandomNumberGenerator]::Create()
    try { $rng.GetBytes($bytes);$env:TSPR3_WEBHOOK_SECRET=[Convert]::ToBase64String($bytes) } finally { $rng.Dispose();[Array]::Clear($bytes,0,$bytes.Length) }
}
$env:TSPR3_ALLOW_PUBLISH=if($EnablePublish) {'1'} else {'0'}
$state=if($env:TSPR3_PIPELINE_HOME) {$env:TSPR3_PIPELINE_HOME} else {Join-Path $env:LOCALAPPDATA 'TSPR3\pipeline'}
[void][IO.Directory]::CreateDirectory($state)
if($EnablePublish -and !(Test-Path -LiteralPath (Join-Path $state 'release-secrets.json') -PathType Leaf)) {
    throw 'Run scripts\release\set-secrets.ps1 with fresh credentials before enabling publication.'
}
$node=(Get-Command node -ErrorAction Stop).Source;$script=Join-Path $PSScriptRoot 'pipeline.mjs'
$process=Start-Process -FilePath $node -ArgumentList @('"'+$script+'"') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $state 'server.out.log') -RedirectStandardError (Join-Path $state 'server.err.log')
Write-Host "Server PID: $($process.Id). Use request.mjs from this same terminal. Logs: $state"
