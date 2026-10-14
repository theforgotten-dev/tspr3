param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$tspr3Gpg = 'C:\Program Files\Git\usr\bin\gpg.exe'
$tspr3Fingerprint = '3DA8BCEF0D9A7CADEE2390E1E7E1DFF927C45BD3'
$tspr3KeyHome = Join-Path $env:USERPROFILE '.tspr3-gnupg'
if (!(Test-Path -LiteralPath $tspr3Gpg)) { throw 'GPG de Git no encontrado.' }
if (!(Test-Path -LiteralPath $tspr3KeyHome)) { throw 'Almacen GPG de TSPR3 no encontrado.' }
$tspr3MsysHome = '/' + $tspr3KeyHome.Substring(0,1).ToLowerInvariant() + $tspr3KeyHome.Substring(2).Replace('\','/')
$tspr3Previous = @{}
foreach ($name in @('GNUPGHOME','CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
    $tspr3Previous[$name] = [Environment]::GetEnvironmentVariable($name,'Process')
}
try {
    $env:GNUPGHOME = $tspr3MsysHome
    & $tspr3Gpg --homedir $tspr3MsysHome --batch --list-secret-keys $tspr3Fingerprint
    if ($LASTEXITCODE -ne 0) { throw 'La clave de firma no esta disponible.' }
    if ($CheckOnly) {
        & "$PSScriptRoot\build-maven.ps1" -Goals @('-Pcentral-release','validate')
        return
    }
    foreach ($item in @(
        @{Name='CENTRAL_TOKEN_USERNAME';Prompt='Token username de Sonatype'},
        @{Name='CENTRAL_TOKEN_PASSWORD';Prompt='Token password de Sonatype'},
        @{Name='MAVEN_GPG_PASSPHRASE';Prompt='Contrasena de la clave GPG'}
    )) {
        $secret = Read-Host $item.Prompt -AsSecureString
        try {
            $value = [Net.NetworkCredential]::new('', $secret).Password
            if ([string]::IsNullOrEmpty($value)) { throw 'El valor no puede estar vacio.' }
            [Environment]::SetEnvironmentVariable($item.Name,$value,'Process')
        } finally {
            $value = $null
            $secret.Dispose()
        }
    }
    & "$PSScriptRoot\build-maven.ps1" -Goals @(
        '-Pcentral-release', "-Dgpg.executable=$tspr3Gpg",
        "-Dgpg.keyname=$tspr3Fingerprint", 'deploy'
    )
    Write-Host 'Subida completada. Revisa el deployment en Sonatype Central antes de publicarlo.'
} finally {
    foreach ($name in $tspr3Previous.Keys) {
        [Environment]::SetEnvironmentVariable($name,$tspr3Previous[$name],'Process')
    }
}
