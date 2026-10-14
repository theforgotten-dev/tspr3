# Verificación local C — 13 de octubre de 2026

Windows x64, AMD Ryzen 5 4600G, MSVC tools 14.44.35207,
C17, `/O2 /W4 /WX`. Compilación sin advertencias y salida correcta:
7 casos deterministas + 5.010 aleatorios para SUM y FULL, más argumentos inválidos.

Medianas de cinco repeticiones a 100.000 elementos, en milisegundos:

| Distribución | FULL | qsort |
|---|---:|---:|
| Aleatoria | 6,206 | 8,679 |
| Ascendente | 0,448 | 2,523 |
| Descendente | 0,455 | 2,588 |
| Duplicados | 2,941 | 1,686 |
| Rangos disjuntos por terna | 0,391 | 2,702 |
| Bloques intercalados | 1,821 | 6,247 |

FULL fue más rápido en varias distribuciones de esta ejecución y más lento con
duplicados. Esto no demuestra superioridad general ni que la terna sea la causa
del beneficio: falta comparar con el mismo merge sort sin atajos y con otros
tamaños de bloque inicial. Tampoco hay evidencia de aceleración paralela.

Los tiempos completos de los tres tamaños se guardaron en
`build/verification.txt`. Se pueden regenerar con:

```powershell
.\build.ps1 -Task legacy -Benchmark | Tee-Object -FilePath build\verification.txt
```

Limitaciones y condiciones de medición descritas en `README.md`.

## Incorporación de FREQ

Nueva compilación con las mismas opciones: 10 casos deterministas + 5.010
aleatorios para SUM, FULL, FREQ y su índice, además de argumentos inválidos.
Las 18 configuraciones del benchmark verificaron salidas FULL/FREQ idénticas a
`qsort` en cada repetición. La última ejecución reemplaza `build/verification.txt`.

Medianas de cinco repeticiones a 100.000 elementos, en milisegundos:

| Distribución | Valores distintos k | FULL | FREQ | qsort |
|---|---:|---:|---:|---:|
| Aleatoria | 97.598 | 6,750 | 16,657 | 8,603 |
| Ascendente | 100.000 | 0,388 | 16,717 | 2,410 |
| Descendente | 100.000 | 0,478 | 16,495 | 2,509 |
| Duplicados | 8 | 3,342 | 0,578 | 1,738 |
| Rangos disjuntos por terna | 100.000 | 0,433 | 16,819 | 2,596 |
| Bloques intercalados | 100.000 | 1,806 | 16,519 | 6,187 |

Con ocho claves, FREQ mejora el caso que motivó la variante. Con casi todos los
valores distintos, el índice añade trabajo y es más lento. Estas mediciones no
definen un umbral universal de selección; falta explorar valores intermedios de
`k/n`, más repeticiones y dispersión.

## Optimización posterior

Se incorporó AUTO con selección basada en regresión de costos, conteo limitado
y rutas lineales para entrada monótona o ternas con rangos separados.
Pasaron 5.026 casos y 66 configuraciones de benchmark. Fórmulas, calibración,
resultados posteriores y límites: [OPTIMIZACION.md](LEGACY-OPTIMIZATION.md).

## Versión afinada con un millón de elementos

Se añadieron radix signed, conteo directo, tramos ascendentes/descendentes,
extracción de un valor dominante y límites de trabajo del índice AUTO.
Pasaron 5.033 casos básicos, 750 ordenamientos de un millón y 30 verificaciones
SUM. AUTO fue más rápida que FULL en las 150 entradas emparejadas; mediana del
cociente FULL/AUTO: 8,284. La regresión anterior ya no gobierna la selección.
Resultados vigentes, fórmulas y limitaciones: [MILLON.md](LEGACY-MILLION.md).
