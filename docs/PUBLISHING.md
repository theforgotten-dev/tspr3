# Publicación Maven Central

Coordenadas preparadas: `io.github.theforgotten-dev:tspr3:0.1.1`.
El usuario GitHub activo se verificó con `gh api user`. Esto no equivale a una
verificación del namespace en Sonatype. El propietario confirmó su estado
Verified el 14 de octubre de 2026. La URL `theforgotten-dev/tspr3` del POM
es el repositorio público que aloja ambas bibliotecas.

## Construcción local verificable

```powershell
.\build.ps1 -Task maven
```

El script descarga Maven 3.9.16 desde el repositorio oficial de Apache y verifica
su SHA-512. Las herramientas y el caché quedan en `.tools`, ignorado por Git.
Usa un JDK 17+ instalado y lo establece solo durante el comando, restaurando
`JAVA_HOME`. No cambia configuración global. Compila a Java 8 mediante `release=8`.
Solo se suprimen las advertencias sobre esa versión de destino antigua; las
advertencias del código continúan tratándose como errores.

Artefactos en `java/target`:

- `tspr3-0.1.1.jar`
- `tspr3-0.1.1-sources.jar`
- `tspr3-0.1.1-javadoc.jar`

Maven ejecuta las 23.352 entradas de prueba reales, no se limita a un resultado
de Surefire sin pruebas. El perfil normal no firma ni sube archivos.

## Estado de preparación

La clave GPG y las firmas verificadas anteriormente corresponden a 0.1.0.
Para 0.1.1 se verificaron compilacion, pruebas, fuentes, Javadoc y metadatos;
todavia no se firmo ni se subio un bundle. El token y la contrasena GPG no
estan disponibles en la sesion de preparacion.
La version 0.1.0 esta publicada; 0.1.1 requiere una nueva firma y un nuevo deployment.

La licencia Apache-2.0 fue elegida por el propietario y está incluida en
el repositorio, el paquete C/C++ y el JAR Java.

Sonatype exige metadatos, fuentes, Javadoc y firmas; estas condiciones proceden
de sus [requisitos oficiales](https://central.sonatype.org/publish/requirements/).
La comprobación de namespace se describe en
[Registro de namespace](https://central.sonatype.org/register/namespace/).

No se generaron credenciales de publicación.
`config/maven/settings.example.xml` solo referencia variables de entorno;
ningún token ni contraseña se guarda en el repositorio.

## Flujo de publicación preparado

Para configurar la clave y el token localmente, sigue [MAVEN-SETUP.md](MAVEN-SETUP.md).

El perfil `central-release` comprueba licencia y namespace antes de firmar.
`maven-gpg-plugin` firma en verify; `central-publishing-maven-plugin` sube un
bundle en deploy, genera checksums y tiene `autoPublish=false`. Por tanto, el
despliegue queda para revisión y publicación manual en el Portal, según la
[documentación oficial](https://central.sonatype.org/publish/publish-portal-maven/).

Ejemplo para ejecutar únicamente después de completar esos requisitos:

```powershell
.\scripts\build-maven.ps1 -Goals @(
  '-Pcentral-release',
  'deploy'
)
```

Los nombres del token son `CENTRAL_TOKEN_USERNAME` y `CENTRAL_TOKEN_PASSWORD`.
El GPG instalado y sus credenciales se administran fuera del repositorio.
Para el almacén y la clave configurados, usa `scripts/publish-central.ps1`,
que solicita secretos con entrada oculta y configura la ruta de GPG.
Tambien admite esas variables ya configuradas en el entorno del proceso.
No pegues secretos en el chat ni los guardes en archivos versionados.

```powershell
.\scripts\publish-central.ps1 -CheckOnly
.\scripts\publish-central.ps1
```

Los artefactos firmados locales no acreditan por sí solos la publicación:
el metadata publico confirma 0.1.0 como ultima version el 3 de noviembre de 2026.
