# Contribuir

TSPR3 investiga ordenamiento por ternas. Mantén separados los resultados de
BASE, COUNTS y BIN; radix solo es una referencia externa. No presentes una
mejora de AUTO como prueba de superioridad de las ternas.

Antes de abrir un pull request:

```powershell
.\build.ps1 -Task c
.\build.ps1 -Task java -Native
.\build.ps1 -Task maven
node --test tests/release/pipeline.test.mjs
```

Los cambios algorítmicos deben conservar entrada, longitud, multiconjunto y los
payloads de registros. Añade una referencia independiente y un caso que falle
antes del cambio. Los benchmarks deben publicar distribución, semillas, tamaño,
hardware y rutas comparadas. No mezcles medidas instrumentadas con históricas.

Las APIs internas no tienen garantía de compatibilidad. Una API pública nueva,
incluida la genérica, requiere diseño documentado y un incremento minor `0.x`.
