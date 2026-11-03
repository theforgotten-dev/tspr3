# Publicar una versión experimental

El flujo es transaccional por fases y nunca reintenta una publicación a ciegas:

1. Preparar versión y changelog en un clon aislado.
2. Ejecutar C/C++, Java/JNI, Maven, fuentes, Javadoc y SBOM.
3. Crear commit local y subir el bundle firmado a Central como `USER_MANAGED`.
4. Esperar `VALIDATED` y guardar el `deploymentId`.
5. Crear tag GPG firmado y hacer push atómico de `main` y tag.
6. Publicar ese deployment, esperar `PUBLISHED` y comparar el JAR público.
7. GitHub Actions crea los artefactos de release y su atestación.

Configura credenciales nuevas fuera del repositorio:

```powershell
.\scripts\release\set-secrets.ps1
.\scripts\release\start.ps1 -EnablePublish
node scripts/release/request.mjs publish patch
```

Para una comprobación sin efectos externos:

```powershell
.\scripts\release\start.ps1
node scripts/release/request.mjs check patch
```

Si Git ya está sincronizado y Central queda en `VALIDATED` o `PUBLISHING`, usa el
mismo ID; no incrementes la versión:

```powershell
.\scripts\release\resume.ps1 -DeploymentId UUID
```

Los estados `central-validated`, `git-synchronized`, `central-published` y
`published-and-verified` se conservan bajo `%LOCALAPPDATA%\TSPR3\pipeline`.
