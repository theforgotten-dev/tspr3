# Pipeline local de releases

Requiere Windows, Node 24+, Git autenticado para push, MSVC 2022 y JDK 17+.
La clave GPG existente permanece fuera del repositorio. El token y la passphrase
se cifran mediante DPAPI para el usuario Windows actual, fuera del checkout.

## Arranque y prueba sin publicar

Desde la raiz del proyecto, en PowerShell:

```powershell
.\scripts\release\start.ps1
node scripts/release/request.mjs check patch
node scripts/release/request.mjs status ID_DEVUELTO
```

El arranque genera un secreto HMAC aleatorio en el entorno de esa terminal,
heredado por el servidor oculto. Ejecutar el cliente en la misma terminal.
No guarda secretos en disco. Escucha solo en `127.0.0.1:8787`.
Logs, snapshots y estado quedan en `%LOCALAPPDATA%\TSPR3\pipeline`.
La pagina `http://127.0.0.1:8787/` muestra estado y bloqueo de publicacion;
`/health` devuelve el mismo estado como JSON. Ambas son de solo lectura.

Para habilitar publicaciones, detener primero la instancia anterior y arrancar:

```powershell
.\scripts\release\set-secrets.ps1
.\scripts\release\start.ps1 -EnablePublish
node scripts/release/request.mjs publish patch
```

`set-secrets.ps1` solicita token username, token password y passphrase GPG con
entrada oculta. El servidor no recibe secretos: el worker los descifra solo para
la fase de firma/publicacion y los elimina de su entorno al finalizar.
Tambien admite `minor` y `major`. Arrancar no inicia una release.

## Flujo

1. Clona `main` de `https://github.com/theforgotten-dev/tspr3.git` en un directorio nuevo por job.
2. Incrementa SemVer en ambos POM, CMake, las versiones C/Java y los scripts de empaquetado.
3. Rechaza versiones presentes en Central o tags remotos existentes. Un error de red no se interpreta como ausencia.
4. Ejecuta C/C++, consumidores instalados, Java/JNI y Maven `clean verify`.
5. En modo `check` termina sin commit, push ni subida a Maven.
6. En modo `publish` crea el commit local y sube a Central sin publicar; espera `VALIDATED` y conserva el deployment ID.
7. Crea un tag GPG firmado y sincroniza `main` y tag mediante push atomico sin force.
8. Publica exactamente ese deployment, espera `PUBLISHED` y compara por SHA-256 el JAR publico con el local.

El checkout original y sus archivos sin seguimiento permanecen intactos.
Los snapshots se conservan para investigar fallos; no hay limpieza automatica.
El main local original puede quedar detras del remoto; actualizarlo mediante
`git pull --ff-only` cuando sus cambios locales lo permitan.

## Contrato del webhook

`POST /release`: JSON `{"id":"identificador-unico","bump":"patch","mode":"check"}`.
`GET /jobs/ID`: consulta autenticada. Cuerpo maximo 4096 bytes.

Cabeceras `x-tspr3-timestamp` (epoch en milisegundos) y `x-tspr3-signature`
(HMAC-SHA256 hexadecimal). Se firma exactamente:

```text
timestamp + "." + METHOD + " " + URL + "\n" + BODY
```

Acepta cinco minutos de desfase y compara firmas en tiempo constante.
Un ID repetido devuelve el resultado previo y no repite la release.
Un job concurrente recibe 409. Publicar sin habilitarlo recibe 403.
Tras 20 firmas inválidas por minuto, el servidor local responde 429.
El cliente calcula las firmas; no transmite credenciales Maven.
No se expone a Internet ni crea tuneles. Un origen remoto requiere configurar
acceso autenticado adicional: GitHub no puede acceder a localhost directamente.

## Fallos y recuperacion

No hay reintentos automaticos. Si el estado es `central-validated`, Git todavia
no esta sincronizado. Si es `git-synchronized`, reanudar el mismo deployment con
`scripts/release/resume.ps1 -DeploymentId UUID`. No volver a subir una version
publicada ni incrementar para ocultar el fallo. Un timeout puede terminar en
publicacion; consultar el deployment antes de repetir una operacion.

`server.lock` impide dos servidores en el mismo directorio. Para detener una
instancia oculta usar `Stop-Process -Id PID`. Antes de borrar manualmente el
lock, comprobar que no quedan workers ejecutandose. Un cierre abrupto conserva
jobs `running`: revisarlos antes de reiniciar. No se recuperan automaticamente.
Como alternativa, ejecutar `node scripts/release/pipeline.mjs` en primer plano
con las variables necesarias y detener con Ctrl+C para liberar el lock.

Los logs y el entorno del servidor son privados del usuario Windows. No usar
Maven `-X`, ni compartir logs sin revisarlos. Variables opcionales:
`TSPR3_PIPELINE_HOME` y `TSPR3_WEBHOOK_PORT`. `TSPR3_ALLOW_PUBLISH=1` habilita
publicacion al arrancar manualmente.

```powershell
node --test tests/release/pipeline.test.mjs
```

## Objetivos operacionales

Servicio local, usuario único, una release simultánea. Objetivo de disponibilidad:
99% durante una sesión manual; p99 menor a 250 ms para dashboard, health y
aceptación del job (la compilación continúa asíncrona). RPO 0 para transiciones
persistidas y RTO de 30 minutos usando el checkout, estado y deployment ID.

## Despliegue con `main` protegido

La rama `main` exige commits firmados, los checks de Windows, Linux y CodeQL,
una aprobacion de PR distinta del autor del ultimo push y conversaciones
resueltas. No permite force-push ni eliminacion. Estas reglas tambien se aplican
a administradores.

El modo seguro actual se divide en dos etapas:

1. Ejecutar `node scripts/release/request.mjs check patch` para preparar y probar
   la siguiente version en un checkout aislado. Esta operacion no publica ni
   modifica Git.
2. Crear una rama de release desde `main`, aplicar el incremento y CHANGELOG,
   ejecutar las mismas pruebas, crear un commit firmado y abrir un PR.
3. Esperar los tres checks obligatorios y una revision externa. Cualquier push
   nuevo descarta la aprobacion anterior y debe revisarse nuevamente.
4. Despues del merge, crear el tag firmado desde el commit integrado, ejecutar
   el deployment `USER_MANAGED` de Central, esperar `VALIDATED`, publicar ese
   mismo deployment y verificar el SHA-256 del JAR publico.
5. El tag activa `.github/workflows/release-artifacts.yml`, que vuelve a construir,
   genera checksums y SBOM, atestigua procedencia y crea el GitHub Release.

El comando `request.mjs publish` implementa el flujo transaccional original,
pero su push directo a `main` es incompatible con la proteccion actual. No debe
usarse hasta dividir `run.ps1` en una fase `prepare` que abra el PR y una fase
`publish` que solo opere sobre el commit ya integrado y aprobado. No se debe
agregar un bypass personal para conservar la revision independiente.

### Automatizacion recomendada

Mantener el webhook en `127.0.0.1` para validaciones locales. Para automatizar
la publicacion, usar un workflow manual (`workflow_dispatch`) con un entorno
GitHub llamado `maven-central` que requiera aprobador. El workflow debe aceptar
un tag SemVer ya integrado, descargar secretos desde el almacén del entorno,
ejecutar la subida `USER_MANAGED`, conservar el deployment ID como artefacto y
publicar solo despues de la aprobacion del entorno. Nunca almacenar tokens,
passphrases, `settings.xml`, keyrings ni archivos DPAPI en el repositorio.

Antes de habilitar esa automatizacion, rotar el token de Central y la passphrase
GPG que fueron expuestos. Configurar los reemplazos mediante
`scripts/release/set-secrets.ps1`; el archivo resultante permanece en
`%LOCALAPPDATA%\TSPR3\pipeline\release-secrets.json` y no se copia al checkout.
