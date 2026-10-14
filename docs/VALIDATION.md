# Validación de la organización y arquitectura

Las medidas históricas se conservaron en `benchmarks/results` y los informes
de investigación en `docs/research`. La refactorización modifica organización
y despacho, manteniendo los contratos de salida. Los tiempos anteriores no se
presentan como una medición nueva del código refactorizado.

Comprobaciones locales:

- CMake Release/MSVC: API C, adaptador C++, 90.399 entradas del núcleo y política de selección.
- Consumidores C y C++ independientes que encuentran el paquete instalado.
- Java 8: 23.352 entradas, tanto Java puro como JNI, y un consumidor externo del JAR.
- Maven con JDK 21 y bytecode Java 8: pruebas reales, JAR, fuentes y Javadoc.
- Prueba adicional Java 21/JNI con entradas de dos millones para datos amplios, ocho claves y ternas de rangos próximos.
- El ensayo legacy sigue compilando y conserva sus 5.033 casos.
- `central-release validate` rechaza como se esperaba la licencia sin definir y el namespace sin verificar, antes de firmar o subir artefactos.

Los comandos de reproducción son:

```powershell
.\build.ps1 -Task all
.\build.ps1 -Task maven
.\build.ps1 -Task java -Native -Large
.\build.ps1 -Task tri -Sanitize
```

CI Windows está publicada en `.github/workflows/ci.yml` y se ejecutó en GitHub.
El primer run falló al solicitar Visual Studio 2022 sobre `windows-latest`.
Se fijó el runner a `windows-2022`; las ejecuciones posteriores aprobaron C/C++,
Java/JNI y Maven. [Run del commit de v0.1.0](https://github.com/theforgotten-dev/tspr3/actions/runs/38603341161).
No se declara validación Linux/macOS.

El perfil `central-release validate` aprobó después de definir Apache-2.0 y
registrar el namespace Verified confirmado por el propietario. No se ejecutó
`central-release deploy` ni se publicaron componentes en Central. El código y
los paquetes están disponibles en GitHub Releases. Estado completo:
[PROYECTO-Y-DESPLIEGUE.md](PROYECTO-Y-DESPLIEGUE.md).
