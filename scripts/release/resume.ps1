param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-f-]{36}$')][string]$DeploymentId,
    [string]$SecretsFile=(Join-Path $env:LOCALAPPDATA 'TSPR3\pipeline\release-secrets.json')
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'secrets.ps1');. (Join-Path $PSScriptRoot 'central-api.ps1')
try { Import-Tspr3ReleaseSecrets -Path $SecretsFile;Publish-Tspr3CentralDeployment -DeploymentId $DeploymentId -TimeoutSeconds 1800 }
finally { Clear-Tspr3ReleaseSecrets }
