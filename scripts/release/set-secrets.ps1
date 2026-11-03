param([string]$Path=(Join-Path $env:LOCALAPPDATA 'TSPR3\pipeline\release-secrets.json'))
$ErrorActionPreference='Stop';$directory=[IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($Path));[void][IO.Directory]::CreateDirectory($directory)
$protected=[ordered]@{}
foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
    $value=Read-Host $name -AsSecureString
    try { if([Net.NetworkCredential]::new('',$value).Password.Length -eq 0) { throw "$name cannot be empty." };$protected[$name]=ConvertFrom-SecureString $value }
    finally { $value.Dispose() }
}
$temporary="$Path.tmp";$protected | ConvertTo-Json | Set-Content -LiteralPath $temporary -Encoding UTF8;Move-Item -LiteralPath $temporary -Destination $Path -Force
Write-Host "Encrypted for the current Windows user: $Path"
