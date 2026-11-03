function Import-Tspr3ReleaseSecrets([Parameter(Mandatory)][string]$Path) {
    if(!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw 'Encrypted release secrets are not configured.' }
    $protected=Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) {
        $cipher=[string]$protected.$name;if(!$cipher) { throw "Missing protected secret $name" }
        $secure=ConvertTo-SecureString $cipher
        try { $plain=[Net.NetworkCredential]::new('',$secure).Password;[Environment]::SetEnvironmentVariable($name,$plain,'Process') }
        finally { $plain=$null;$secure.Dispose() }
    }
}
function Clear-Tspr3ReleaseSecrets {
    foreach($name in @('CENTRAL_TOKEN_USERNAME','CENTRAL_TOKEN_PASSWORD','MAVEN_GPG_PASSPHRASE')) { [Environment]::SetEnvironmentVariable($name,$null,'Process') }
}
