# Pruebas Java/JNI - 13 de octubre de 2026

Snapshot anterior a la organización del repositorio. La huella corresponde al
JAR probado en ese momento; validación actual en [VALIDATION.md](../docs/VALIDATION.md).

Se ejecuto `build-java.ps1 -Benchmark` con HotSpot 1.8.0_502 x64.

Pasaron 23.352 entradas de prueba en Java puro y otras 23.352 con JNI.
El consumidor se compilo y ejecuto contra el JAR. Las 315 salidas de dos millones
de elementos coincidieron con la referencia: nueve escenarios, cinco semillas,
siete rutas. AUTO incluye la condicion corregida para repeticiones.

Medianas en milisegundos; tiempos individuales, minimos y maximos en
`benchmarks/results/java_raw.csv` y `benchmarks/results/java_summary.csv`.

| Escenario | Java A | Java B | Java C | Java AUTO | JNI A | JNI B | JNI C |
|---|---:|---:|---:|---:|---:|---:|---:|
| wide | 214.32 | 347.28 | 401.02 | 240.72 | 211.19 | 260.22 | 313.95 |
| bounded | 229.67 | 368.22 | 403.32 | 232.53 | 213.65 | 254.59 | 346.60 |
| ascending | 33.26 | 90.95 | 150.73 | 32.06 | 40.34 | 145.44 | 227.21 |
| descending | 31.79 | 89.69 | 150.19 | 34.12 | 41.98 | 165.50 | 248.14 |
| k8 | 109.70 | 74.24 | 101.17 | 75.00 | 108.63 | 76.27 | 118.96 |
| k2048 | 170.75 | 188.36 | 219.41 | 175.89 | 151.55 | 152.09 | 200.66 |
| skew99 | 73.79 | 37.36 | 76.27 | 41.66 | 72.05 | 44.49 | 104.74 |
| normal | 216.69 | 350.80 | 408.46 | 212.67 | 186.35 | 237.95 | 306.67 |
| local_normal | 133.70 | 250.89 | 919.50 | 129.64 | 113.23 | 157.50 | 949.61 |

Se calentaron las rutas antes de medir y roto el orden de ejecucion.
Los tiempos incluyen reservas, GC Java y copias JNI; no generacion ni referencia.
No se fijo afinidad o carga del sistema ni se hizo analisis de significancia.

La JVM emitio advertencias con `-Xcheck:jni`; no hubo fallos de pruebas.
La advertencia de `System.initProperties` tambien se reprodujo en el consumidor
Java puro durante la entrega anterior. No se afirma que JNI estuviera libre
de advertencias.

JAR probado: `dist/tspr3-java-0.1.0.jar`. SHA-256: `2bfe43fe846c26e52dae459ceada8b8274174133bd8f43722d7e66fe93adb119`.
