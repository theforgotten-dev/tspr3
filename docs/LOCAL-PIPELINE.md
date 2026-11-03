# Pipeline local de releases

Requiere Windows, Node 24+, Git autenticado para push, MSVC 2022 y JDK 17+.
La clave GPG existente permanece fuera del repositorio. Introducir credenciales
nuevas en la consola; reemplazar los secretos expuestos en el chat.

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
.\scripts\release\start.ps1 -EnablePublish
node scripts/release/request.mjs publish patch
```

Solicita token username, token password y passphrase GPG con entrada oculta.
Los secretos solo se heredan al servidor y se quitan de la terminal.
Tambien admite `minor` y `major`. Arrancar no inicia una release.

## Flujo

1. Clona `main` de `https://github.com/theforgotten-dev/tspr3.git` en un directorio nuevo por job.
2. Incrementa SemVer en ambos POM, CMake, las versiones C/Java y los scripts de empaquetado.
3. Rechaza versiones presentes en Central o tags remotos existentes. Un error de red no se interpreta como ausencia.
4. Ejecuta C/C++, consumidores instalados, Java/JNI y Maven `clean verify`.
5. En modo `check` termina sin commit, push ni subida a Maven.
6. En modo `publish` crea commit y tag anotado y sincroniza ambos con push atomico sin force. Una actualizacion concurrente de main rechaza ambos.
7. Firma y publica en Central con `autoPublish=true` y `waitUntil=published`.
8. Descarga el JAR publico y compara SHA-256 con el artefacto local.

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
El cliente calcula las firmas; no transmite credenciales Maven.
No se expone a Internet ni crea tuneles. Un origen remoto requiere configurar
acceso autenticado adicional: GitHub no puede acceder a localhost directamente.

## Fallos y recuperacion

No hay reintentos automaticos de publicacion. Despues de `git-synchronized`
pueden existir commit/tag publicados y Maven pendiente. Consultar log,
`ID.release.json` y Portal antes de actuar. Entrar al checkout conservado y
ejecutar `scripts/publish-central.ps1` para terminar la misma version si todavia
no se subio. Si la subida ya fue aceptada, revisar/publicar ese deployment en
Central. No volver a subir una version publicada ni generar otra para ocultar
el fallo. La espera de Central puede terminar por timeout y aun publicarse.

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
