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

CI Windows está preparada en `.github/workflows/ci.yml`; no se ejecutó aún en
GitHub porque no existe un remoto publicado. No se declara validación Linux/macOS.
El perfil `central-release` no se ejecutó ni se publicaron componentes en Central.
