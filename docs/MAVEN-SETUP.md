# Firma y publicación desde PowerShell

El repositorio está publicado bajo Apache-2.0. El propietario confirmó
`io.github.theforgotten-dev` como Verified en Sonatype. La subida a Central
requiere una clave de firma y el token del Portal.

## 1. Crear una clave protegida

Abre PowerShell en el repositorio y ejecuta:

```powershell
Set-Location E:\proys\algoritmoOrdenamiento
$tspr3Gpg = 'C:\Program Files\Git\usr\bin\gpg.exe'
$tspr3KeyHome = Join-Path $env:USERPROFILE '.tspr3-gnupg'
New-Item -ItemType Directory -Path $tspr3KeyHome -Force | Out-Null
$tspr3Common = Join-Path $tspr3KeyHome 'common.conf'
if (!(Test-Path -LiteralPath $tspr3Common)) {
    Set-Content -LiteralPath $tspr3Common -Value '# File keyring for TSPR3; no keyboxd' -Encoding ASCII
}
$tspr3Drive = $tspr3KeyHome.Substring(0,1).ToLowerInvariant()
$env:GNUPGHOME = '/' + $tspr3Drive + $tspr3KeyHome.Substring(2).Replace('\','/')
& $tspr3Gpg --full-generate-key
```

Elige RSA y RSA, 4096 bits, una validez de 2 años, tu nombre y el correo
que quieras asociar públicamente a las firmas. Introduce una contraseña
segura en el diálogo de GPG. Conserva una copia privada de respaldo de tu
clave y del certificado de revocación fuera del repositorio.

Este directorio es exclusivo para TSPR3 y no cambia el directorio GPG general.
Reestablece `GNUPGHOME` con la misma ruta si abres otra sesión para firmar.
El GPG de Git usa rutas MSYS como `/c/Users/juan/.tspr3-gnupg`; el bloque
convierte la ruta Windows antes de entregársela al ejecutable.
Se usa un `common.conf` sin `use-keyboxd`: el GPG incluido en Git puede fallar
si intenta iniciar `/usr/lib/gnupg/keyboxd`, que no está instalado. Esta
preparación permite usar el almacén de archivos; no borra ni migra claves
existentes. La consulta de claves con este directorio se verificó localmente.
Véase [configuración GnuPG](https://gnupg.org/documentation/manuals/gnupg/GPG-Configuration.html).

Obtén la huella completa y publica únicamente la clave pública:

```powershell
& $tspr3Gpg --list-secret-keys --keyid-format long
$tspr3Fingerprint = Read-Host 'Huella completa de la clave de firma'
& $tspr3Gpg --keyserver hkps://keyserver.ubuntu.com --send-keys $tspr3Fingerprint
if ($LASTEXITCODE -ne 0) { throw 'No se pudo publicar la clave pública' }
```

La publicación de la clave pública permite comprobar las firmas de los
artefactos. Sigue los [requisitos oficiales de GPG](https://central.sonatype.org/publish/requirements/gpg/).

## 2. Generar el token

En [Sonatype Central](https://central.sonatype.com), pulsa tu usuario,
**View Account** y **Generate User Token**. Guarda el username y password
generados en tu gestor de contraseñas; no son tu contraseña de GitHub.
Consulta la [guía oficial del token](https://central.sonatype.org/publish/generate-portal-token/).

## 3. Firmar y subir para revisión

En la misma sesión de PowerShell, introduce los secretos mediante entrada
oculta. Las variables solo se usan durante el comando y se restauran al terminar.
No pegues los secretos directamente dentro de comandos ni los envíes por chat.

```powershell
$tspr3PreviousUser = $env:CENTRAL_TOKEN_USERNAME
$tspr3PreviousPassword = $env:CENTRAL_TOKEN_PASSWORD
$tspr3PreviousPassphrase = $env:MAVEN_GPG_PASSPHRASE
try {
    $tspr3Secret = Read-Host 'Token username de Central' -AsSecureString
    $env:CENTRAL_TOKEN_USERNAME = [Net.NetworkCredential]::new('', $tspr3Secret).Password
    $tspr3Secret = Read-Host 'Token password de Central' -AsSecureString
    $env:CENTRAL_TOKEN_PASSWORD = [Net.NetworkCredential]::new('', $tspr3Secret).Password
    $tspr3Secret = Read-Host 'Contraseña de la clave GPG' -AsSecureString
    $env:MAVEN_GPG_PASSPHRASE = [Net.NetworkCredential]::new('', $tspr3Secret).Password
    .\scripts\build-maven.ps1 -Goals @(
        '-Pcentral-release',
        "-Dgpg.executable=$tspr3Gpg",
        "-Dgpg.keyname=$tspr3Fingerprint",
        'deploy'
    )
} finally {
    $env:CENTRAL_TOKEN_USERNAME = $tspr3PreviousUser
    $env:CENTRAL_TOKEN_PASSWORD = $tspr3PreviousPassword
    $env:MAVEN_GPG_PASSPHRASE = $tspr3PreviousPassphrase
    if ($tspr3Secret) { $tspr3Secret.Dispose() }
}
```

El perfil ejecuta pruebas, produce JAR, fuentes y Javadoc, firma y sube el
bundle. Como `autoPublish=false`, revisa el deployment en **Publish** del
Portal y pulsa **Publish** cuando la validación termine correctamente.
La disponibilidad en GitHub Releases no implica disponibilidad en Central.
