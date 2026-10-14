# TSPR3: catálogo de comandos PowerShell

Referencia de los comandos del proyecto, sus scripts y sus parámetros actuales.
Incluye construcción, pruebas, benchmarks, integración, GitHub y Sonatype Central.
Fecha de revisión: 14 de octubre de 2026. No es un catálogo de todos los cmdlets
del lenguaje PowerShell: cubre todas las entradas operativas de TSPR3.

## 1. Carpeta y forma de ejecución

Ejecuta primero el cambio de carpeta y después el script:

```powershell
Set-Location -LiteralPath E:\proys\algoritmoOrdenamiento
Get-Location
Test-Path -LiteralPath .\scripts\publish-central.ps1
```

Desde cualquier carpeta también puedes usar una ruta absoluta:

```powershell
& 'E:\proys\algoritmoOrdenamiento\build.ps1' -Task c
```

`&` invoca un ejecutable o script cuya ruta está entre comillas. `.` seguido de
espacio carga funciones en la sesión. `-Switch` activa una opción; `-Switch:$false`
la desactiva. Los arrays de argumentos se escriben `@('uno','dos')`. Copia solo
el contenido del bloque, sin los indicadores de consola `PS ...>` ni `>>`.

Si Windows bloquea un script descargado, consulta primero:

```powershell
Get-ExecutionPolicy -List
```

Para un archivo de este repositorio que ya revisaste, puedes quitar su marca
de descarga con `Unblock-File -LiteralPath .\scripts\publish-central.ps1`.
No hace falta cambiar la política global para el funcionamiento normal del proyecto.

## 2. Comprobar herramientas

```powershell
$PSVersionTable.PSVersion
git --version
gh --version
java -version
javac -version
jar --version
python --version
Get-Command git,gh,java,javac,jar,python -ErrorAction SilentlyContinue
```

`jar --version` pertenece a JDK modernos; en JDK 8 usa `jar` sin argumentos
para consultar su ayuda. Para el lanzador Python de Windows, la alternativa es
`py -3 --version` y `py -3 archivo.py`.

Funciones compartidas del proyecto:

```powershell
. .\scripts\common\Toolchain.ps1
Get-Tspr3VisualStudio
Get-Tspr3CMake
Get-Tspr3ReleaseJdk
$tspr3Jdk = Get-Tspr3ReleaseJdk
Get-Tspr3JavaFlags $tspr3Jdk
```

La primera función encuentra MSVC mediante `vswhere`; la segunda busca CMake
en PATH o en Visual Studio. La tercera busca JDK 17+ para Maven. La cuarta
devuelve opciones de compilación compatibles con Java 8. Estas funciones no
compilan por sí solas. MSVC `cl.exe` puede no estar en PATH de un PowerShell
ordinario; los scripts inicializan su entorno al compilar.

## 3. Entrada principal: build.ps1

```powershell
.\build.ps1
.\build.ps1 -Task all
.\build.ps1 -Task c
.\build.ps1 -Task java
.\build.ps1 -Task java -Native
.\build.ps1 -Task java -Native -Large
.\build.ps1 -Task java -Benchmark
.\build.ps1 -Task tri
.\build.ps1 -Task tri -Sanitize
.\build.ps1 -Task tri -Benchmark
.\build.ps1 -Task log
.\build.ps1 -Task log -Sanitize
.\build.ps1 -Task log -Benchmark
.\build.ps1 -Task legacy
.\build.ps1 -Task legacy -Benchmark
.\build.ps1 -Task maven
```

| Parámetro | Efecto según tarea |
|---|---|
| `-Task` | `all`, `c`, `java`, `tri`, `log`, `legacy` o `maven`; predeterminado `all`. |
| `-Native` | En `java`, compila y prueba JNI. `all` siempre incluye JNI. |
| `-Large` | En `java`, añade tres entradas de dos millones a los contratos. |
| `-Benchmark` | En `java`, benchmark completo; en `tri`, benchmark de un millón; en `log`, evaluación reservada; en `legacy`, benchmark histórico pequeño. |
| `-Sanitize` | En `tri` o `log`, activa AddressSanitizer de MSVC. |

No todos los flags tienen efecto en todas las tareas. `all` ejecuta C/C++ y
Java/JNI; no incluye Maven ni los benchmarks. El wrapper no expone
`AutoBenchmark`, `InstallPrefix`, `Goals` o `Million` de legacy: usa los scripts
específicos cuando necesites esas opciones. AddressSanitizer sirve para detectar
errores de memoria; sus tiempos no representan un benchmark Release ordinario.

## 4. C/C++: compilar, instalar y probar

```powershell
.\scripts\build-library.ps1
.\scripts\build-library.ps1 -InstallPrefix 'dist\tspr3'
.\scripts\build-library.ps1 -InstallPrefix 'dist\tspr3-custom'
```

`InstallPrefix` indica dónde instalar el paquete; se interpreta desde la raíz
del repositorio si es relativo. El script configura Release x64 con Visual Studio
2022, compila, ejecuta cuatro suites CTest, instala headers y biblioteca, ejecuta
ejemplos y consumidores independientes y genera el ZIP C/C++. No publica en red.

Para ejecutar esas fases manualmente:

```powershell
. .\scripts\common\Toolchain.ps1
$tspr3Cmake = Get-Tspr3CMake
$tspr3Ctest = Join-Path (Split-Path -Parent $tspr3Cmake) 'ctest.exe'
& $tspr3Cmake -S . -B build\library -G 'Visual Studio 17 2022' -A x64
& $tspr3Cmake --build build\library --config Release
& $tspr3Ctest --test-dir build\library -C Release --output-on-failure
& $tspr3Ctest --test-dir build\library -C Release -R library_cpp --output-on-failure
& $tspr3Cmake --install build\library --config Release --prefix dist\tspr3
& .\build\library\Release\tspr3_example_c.exe
& .\build\library\Release\tspr3_example_cpp.exe
```

Consumidor del paquete instalado:

```powershell
$tspr3Prefix = (Resolve-Path -LiteralPath dist\tspr3).Path
& $tspr3Cmake -S tests\consumer -B build\consumer -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$tspr3Prefix"
& $tspr3Cmake --build build\consumer --config Release
& .\build\consumer\Release\consumer_c.exe
& .\build\consumer\Release\consumer_cpp.exe
```

En las fases manuales, comprueba `$LASTEXITCODE` después de cada comando nativo.
Los scripts de construcción ya verifican los códigos y detienen la ejecución
cuando falla una fase. `Resolve-Path` requiere que la instalación exista.

## 5. Java, JNI y pruebas grandes

```powershell
.\scripts\build-java.ps1
.\scripts\build-java.ps1 -Native
.\scripts\build-java.ps1 -Large
.\scripts\build-java.ps1 -Native -Large
.\scripts\build-java.ps1 -Benchmark
.\scripts\build-java.ps1 -AutoBenchmark
.\scripts\build-java.ps1 -Native -AutoBenchmark
```

| Flag | Trabajo adicional |
|---|---|
| Sin flags | Compila Java, genera JAR/fuentes, prueba contratos y consumidor. |
| `-Native` | Compila DLL JNI y ejecuta contratos con `-Xcheck:jni`. |
| `-Large` | Añade tres entradas de dos millones; también a JNI si se solicita. |
| `-Benchmark` | Incluye JNI y ejecuta el benchmark completo Java/JNI. |
| `-AutoBenchmark` | Ejecuta el benchmark Java AUTO únicamente. |

Los flags se pueden combinar; `-Benchmark -AutoBenchmark` ejecuta ambos
benchmarks. El script usa el `javac` disponible en PATH. Las entradas grandes
y los benchmarks necesitan más tiempo y memoria que el recorrido ordinario.

Para seleccionar temporalmente un JDK instalado:

```powershell
. .\scripts\common\Toolchain.ps1
$tspr3SavedPath = $env:PATH
$tspr3SavedJavaHome = $env:JAVA_HOME
try {
    $env:JAVA_HOME = Get-Tspr3ReleaseJdk
    $env:PATH = (Join-Path $env:JAVA_HOME 'bin') + ';' + $tspr3SavedPath
    .\scripts\build-java.ps1 -Native -Large
} finally {
    $env:PATH = $tspr3SavedPath
    $env:JAVA_HOME = $tspr3SavedJavaHome
}
```

Reejecutar programas ya compilados y usar el consumidor externo:

```powershell
java -Xmx2g -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Tspr3Test
java -Xmx2g -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Tspr3Test --large
java -Xcheck:jni -Xmx2g '-Djava.library.path=dist\java-native' -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Tspr3Test --native --large
java -Xms1g -Xmx2g '-Djava.library.path=dist\java-native' -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Benchmark
java -Xms1g -Xmx2g -cp 'build\java-tests;dist\tspr3-java-0.1.0.jar' org.tspr3.Benchmark --auto-only
New-Item -ItemType Directory -Path build\java-client -Force | Out-Null
javac -cp dist\tspr3-java-0.1.0.jar -d build\java-client java\examples\UseTspr3.java
java -cp 'build\java-client;dist\tspr3-java-0.1.0.jar' UseTspr3
jar tf dist\tspr3-java-0.1.0.jar
jar tf dist\tspr3-java-0.1.0-sources.jar
```

Las rutas con `;` deben estar entre comillas en PowerShell. Estos comandos
requieren la construcción previa de sus clases y, para JNI, de la DLL.

## 6. Maven: artefactos y validación sin publicación

```powershell
.\scripts\build-maven.ps1
.\scripts\build-maven.ps1 -Goals @('verify')
.\scripts\build-maven.ps1 -Goals @('clean','verify')
.\scripts\build-maven.ps1 -Goals @('install')
.\scripts\build-maven.ps1 -Goals @('-Pcentral-release','validate')
.\scripts\publish-central.ps1 -CheckOnly
```

`Goals` pasa un array de fases y opciones Maven. `verify` es el predeterminado:
prueba y genera JAR, fuentes y Javadoc. `clean` borra los resultados Maven de
`java/target` antes de regenerarlos, incluidos los bundles y firmas locales.
`install` instala en el caché Maven del proyecto, `.tools/m2`, no necesariamente
en el `.m2` global de otro consumidor. `central-release validate` revisa metadata
sin firmar ni subir. `CheckOnly` también comprueba la clave configurada.

El script usa Maven 3.9.16 y un JDK 17+; configura `JAVA_HOME` durante la llamada
y lo restaura. Los artefactos Maven están en `java/target`. La configuración
de publicación no se activa en un `verify` normal.

## 7. Experimentos de ternas y selector logarítmico

```powershell
.\scripts\build-tri.ps1
.\scripts\build-tri.ps1 -Sanitize
.\scripts\build-tri.ps1 -Million
.\scripts\build-tri.ps1 -Million -Sanitize
python .\tools\analysis\summarize_tri.py
```

`build-tri` siempre prueba el núcleo; `Million` añade el benchmark de ternas
de un millón. `summarize_tri` lee `benchmarks/results/tri_raw.csv` e imprime
su resumen. No convierte esos resultados en una medición nueva.

Recorrido de dos millones y entrenamiento:

```powershell
.\scripts\build-log.ps1
python .\tools\analysis\fit_log_selector.py
.\scripts\validate-log.ps1 -Tests
.\scripts\validate-log.ps1 -Tests -Sanitize
.\scripts\validate-log.ps1
.\scripts\validate-log.ps1 -Sanitize
python .\tools\analysis\analyze_log_selector.py
```

`build-log` mide A/B/C sobre las entradas de dos millones. El ajuste lee
`tri_2m_raw.csv` y **reescribe** `models/c/log_model.json` y
`src/c/internal/tspr3_log_weights.h`. Después se debe recompilar la biblioteca
si se quiere usar esos pesos nuevos. `validate-log -Tests` ejecuta contratos
de perfil/selector; sin `Tests`, ejecuta la evaluación reservada y genera sus
resultados. Los benchmarks pueden reemplazar los CSV de su recorrido.

## 8. Experimentos históricos: legacy

```powershell
.\scripts\build-legacy.ps1
.\scripts\build-legacy.ps1 -Bench
.\scripts\build-legacy.ps1 -Million
.\scripts\build-legacy.ps1 -Bench -Million
python .\experiments\legacy\python\tspr3_verify.py
python .\tools\analysis\summarize_million.py
python .\tools\analysis\analyze_benchmark.py .\benchmarks\results\calibration.csv
```

`Bench` solicita el benchmark histórico pequeño; `Million` añade el de un millón.
El resumen lee `million_raw.csv`. `analyze_benchmark.py` recibe un archivo con
filas de calibración de ocho campos; también puede leer la salida histórica
capturada en `build/optimization.txt`. Estos programas incluyen ensayos anteriores
y no validan superioridad del algoritmo actual frente a radix.

## 9. Capturar resultados y consultar archivos

```powershell
New-Item -ItemType Directory -Path build -Force | Out-Null
.\build.ps1 -Task legacy -Benchmark | Tee-Object -FilePath build\optimization.txt
python .\tools\analysis\analyze_benchmark.py build\optimization.txt
python .\tools\analysis\analyze_log_selector.py | Tee-Object -FilePath build\log-analysis.txt
Get-ChildItem -LiteralPath benchmarks\results
Get-Content -LiteralPath docs\PROYECTO-Y-DESPLIEGUE.md -Encoding UTF8
Import-Csv -LiteralPath benchmarks\results\tri_2m_raw.csv | Select-Object -First 5
```

`Tee-Object` guarda la salida que recibe y también la muestra. En Windows
PowerShell 5.1 puede escribir UTF-16; el analizador histórico contempla ese formato.
Un pipeline no garantiza que `$LASTEXITCODE` represente todas sus fases: para
automatización usa los scripts con control explícito de fallos.

## 10. GPG: almacén, clave pública y firmas

Preparación del almacén exclusivo, sin modificar el GPG general:

```powershell
$tspr3Gpg = 'C:\Program Files\Git\usr\bin\gpg.exe'
$tspr3KeyHomeWindows = Join-Path $env:USERPROFILE '.tspr3-gnupg'
New-Item -ItemType Directory -Path $tspr3KeyHomeWindows -Force | Out-Null
$tspr3Common = Join-Path $tspr3KeyHomeWindows 'common.conf'
if (!(Test-Path -LiteralPath $tspr3Common)) {
    Set-Content -LiteralPath $tspr3Common -Value '# File keyring for TSPR3' -Encoding ASCII
}
$tspr3KeyHome = '/' + $tspr3KeyHomeWindows.Substring(0,1).ToLowerInvariant() + $tspr3KeyHomeWindows.Substring(2).Replace('\','/')
& $tspr3Gpg --version
& $tspr3Gpg --homedir $tspr3KeyHome --list-keys --fingerprint
& $tspr3Gpg --homedir $tspr3KeyHome --list-secret-keys --keyid-format long
```

El GPG de Git usa rutas MSYS como `/c/Users/juan/.tspr3-gnupg`. En esta instalación
la ruta explícita evita consultar el almacén general configurado con `use-keyboxd`.
La ausencia de `keyboxd` causó el fallo inicial de generación. No borres almacenes
ni claves para resolver ese error.

Crear una clave **solo si hace falta una nueva**, publicar su parte pública y
consultarla en el servidor:

```powershell
& $tspr3Gpg --homedir $tspr3KeyHome --full-generate-key
$tspr3Fingerprint = '3DA8BCEF0D9A7CADEE2390E1E7E1DFF927C45BD3'
& $tspr3Gpg --homedir $tspr3KeyHome --keyserver hkps://keyserver.ubuntu.com --send-keys $tspr3Fingerprint
$tspr3KeyUri = 'https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xE7E1DFF927C45BD3'
Invoke-WebRequest -UseBasicParsing -Uri $tspr3KeyUri -OutFile build\central-public-key.asc
& $tspr3Gpg --homedir $tspr3KeyHome --show-keys --fingerprint /e/proys/algoritmoOrdenamiento/build/central-public-key.asc
```

La huella del ejemplo corresponde a la clave actual; una nueva clave tendrá
otra huella. El último comando usa la ruta MSYS de esta instalación: ajústala
si cambias de carpeta. No importa ni exporta una clave privada.

Después de firmar con Maven, verifica los cuatro artefactos:

```powershell
$tspr3ArtifactRoot = '/e/proys/algoritmoOrdenamiento/java/target'
foreach ($name in @('tspr3-0.1.0.jar','tspr3-0.1.0-sources.jar','tspr3-0.1.0-javadoc.jar','tspr3-0.1.0.pom')) {
    & $tspr3Gpg --homedir $tspr3KeyHome --verify "$tspr3ArtifactRoot/$name.asc" "$tspr3ArtifactRoot/$name"
    if ($LASTEXITCODE -ne 0) { throw "Firma invalida: $name" }
}
```

## 11. Sonatype Central: validar y subir

```powershell
.\scripts\publish-central.ps1 -CheckOnly
.\scripts\publish-central.ps1
```

El segundo comando **sube por red**: solicita con entrada oculta username y
password del token y contraseña GPG, ejecuta pruebas, firma y despliega el bundle.
Usa la huella actual del proyecto y el almacén exclusivo. Restaura sus variables
de entorno al terminar. No escribas secretos como argumentos literales.

El perfil usa `autoPublish=false`: revisa el deployment en **Sonatype Central →
Publish** antes de solicitar su publicación definitiva. Una versión publicada
queda inmutable. La guía [MAVEN-SETUP.md](MAVEN-SETUP.md) explica la creación
del token, los secretos y la alternativa manual de `build-maven.ps1 -Goals`.

Consultar disponibilidad pública, sin autenticar:

```powershell
$tspr3PomUri = 'https://repo.maven.apache.org/maven2/io/github/theforgotten-dev/tspr3/0.1.0/tspr3-0.1.0.pom'
try {
    $response = Invoke-WebRequest -UseBasicParsing -Method Head -Uri $tspr3PomUri
    Write-Host "HTTP $($response.StatusCode)"
} catch {
    if ($_.Exception.Response) {
        Write-Host "HTTP $([int]$_.Exception.Response.StatusCode)"
    } else { throw }
}
```

HTTP 200 acredita que el POM se puede consultar; para validar integración
también debe resolverse la dependencia completa en un consumidor. HTTP 404
no determina si existe un deployment pendiente dentro del Portal.

## 12. Git: estado y publicación del código

Para obtener una copia nueva, desde una carpeta donde no exista `tspr3`:

```powershell
git clone https://github.com/theforgotten-dev/tspr3.git
Set-Location -LiteralPath .\tspr3
```

La ruta de esa copia puede diferir de `E:\proys\algoritmoOrdenamiento`; adapta
las rutas absolutas de los ejemplos. No hace falta clonar para trabajar en
el repositorio local que ya existe.

Consultas:

```powershell
git status --short --branch
git diff --stat
git diff
git diff --cached --check
git log -5 --oneline
git remote -v
git ls-remote origin refs/heads/main
git show v0.1.0 --stat
git ls-files
git check-ignore build dist .tools java/target
```

Para publicar cambios revisados, selecciona los archivos explícitamente:

```powershell
git add docs/POWERSHELL.md README.md
git diff --cached
git diff --cached --check
git commit -m 'Document PowerShell commands'
git push origin main
```

Estos comandos modifican Git local y remoto. No añadas carpetas personales
sin revisar; `.vscode` no forma parte del código versionado actual.

## 13. GitHub CLI: repositorio, jobs y Releases

El comando utilizado para la publicación inicial fue:

```powershell
gh repo create theforgotten-dev/tspr3 --public --source . --remote origin --push --description 'TSPR3: triadic integer ordering, compact frequencies and prefix indexing for C, C++ and Java'
```

Se conserva como registro: **no lo repitas**, porque el repositorio y su remoto
ya existen. Crea un repositorio público y envía la rama local; no publica en Maven.

Consultas de autenticación y repositorio:

```powershell
gh auth status
gh auth login
gh api user --jq .login
gh repo view theforgotten-dev/tspr3
gh repo view theforgotten-dev/tspr3 --json url,isPrivate
gh run list --repo theforgotten-dev/tspr3 --limit 10
$tspr3RunId = Read-Host 'ID de la ejecucion de Actions'
gh run view $tspr3RunId --repo theforgotten-dev/tspr3
gh run view $tspr3RunId --repo theforgotten-dev/tspr3 --log-failed
gh run watch $tspr3RunId --repo theforgotten-dev/tspr3 --exit-status --interval 10
gh release list --repo theforgotten-dev/tspr3
gh release view v0.1.0 --repo theforgotten-dev/tspr3
```

Repetir un run solicita una nueva ejecución en GitHub:

```powershell
gh run rerun $tspr3RunId --repo theforgotten-dev/tspr3 --failed
```

Repetir un run antiguo conserva su código/workflow antiguo; para comprobar
la corrección de Visual Studio usa un run del commit con `windows-2022`.
El workflow actual valida C/C++, Java/JNI y Maven; no publica en Central.

Descargar los adjuntos de una versión:

```powershell
New-Item -ItemType Directory -Path build\release-download -Force | Out-Null
gh release download v0.1.0 --repo theforgotten-dev/tspr3 --dir build\release-download
Get-ChildItem -LiteralPath build\release-download
```

Para una **versión nueva**, después de actualizar metadata y aprobar sus pruebas:

```powershell
$tspr3NewVersion = Read-Host 'Nueva version, sin prefijo v'
git tag "v$tspr3NewVersion"
git push origin "v$tspr3NewVersion"
gh release create "v$tspr3NewVersion" --repo theforgotten-dev/tspr3 --verify-tag --title "TSPR3 $tspr3NewVersion" --notes-file build\release-notes.md
```

El archivo de notas debe existir. Crear un Release no adjunta archivos por sí
solo; para subirlos, proporciona sus rutas explícitas a `gh release upload`.
No reutilices `v0.1.0` como si fuera una versión nueva.

Ejemplo de adjuntos para esa versión nueva, cuando ya se hayan generado:

```powershell
gh release upload "v$tspr3NewVersion" "dist\tspr3-java-$tspr3NewVersion.jar" "dist\tspr3-java-$tspr3NewVersion-sources.jar" "dist\tspr3-java-$tspr3NewVersion-javadoc.jar" --repo theforgotten-dev/tspr3
```

Los scripts actuales fijan nombres 0.1.0: debes actualizar versión y empaquetado
antes de usar este ejemplo con otra versión. `gh auth login` inicia el flujo de
autenticación interactiva; `gh auth status` solo consulta su estado.

## 14. Archivos, ZIP y hashes

```powershell
Get-ChildItem -LiteralPath dist
Get-ChildItem -LiteralPath java\target
Get-FileHash -LiteralPath dist\tspr3-java-0.1.0.jar -Algorithm SHA256
Get-FileHash -LiteralPath dist\tspr3-windows-x64-0.1.0.zip -Algorithm SHA256
Get-Content -LiteralPath dist\SHA256SUMS.txt
Compress-Archive -LiteralPath dist\tspr3 -DestinationPath build\tspr3-package.zip -Force
Expand-Archive -LiteralPath build\tspr3-package.zip -DestinationPath build\package-inspection -Force
```

Compara el hash calculado con el checksum distribuido. `-Force` puede reemplazar
archivos de destino; usa carpetas generadas del proyecto. Si `Compress-Archive`
encuentra un archivo bloqueado, no distribuyas un ZIP incompleto; libera el
archivo y repite el empaquetado o usa la API ZIP de .NET con control de errores.

## 15. Diagnóstico por síntoma

| Síntoma | Consulta o acción |
|---|---|
| Script no reconocido desde `system32` | Ejecutar primero `Set-Location`; luego `Test-Path` y el script. |
| Visual Studio 2022 no encontrado | `Get-Tspr3VisualStudio`; en Actions comprobar `windows-2022`. |
| Java equivocado | `Get-Command javac`, `java -version` y selección temporal de JDK. |
| JNI no carga | Comprobar DLL, arquitectura y `-Djava.library.path`. |
| GPG pide `keyboxd` inexistente | Usar `--homedir $tspr3KeyHome` y el almacén exclusivo. |
| Firma no se puede producir | Consultar `--list-secret-keys` en el almacén correcto. |
| Clave pública no encontrada | Consultar el servidor y verificar huella; publicar con `--send-keys`. |
| Token rechazado | Generar/configurar el token de Sonatype; no usar contraseña GitHub. |
| Maven falla | Compartir el error pertinente, sin token, contraseña ni clave privada. |
| Artefacto no disponible | Consultar el POM público y revisar el deployment del Portal. |

No se necesitan comandos de borrado recursivo, cambios de política global ni
reescritura del historial para el recorrido normal. Los comandos de creación
de clave, entrenamiento, benchmarks, tags y publicación son acciones opcionales
con efectos propios; no ejecutes todo este catálogo de una sola vez.
