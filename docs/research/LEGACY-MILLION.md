# AUTO afinado: un millón de elementos por escenario

## Entrega y reproducción

Implementación C17 para enteros `int32_t`, secuencial, con entrada inmutable y
salida reservada por el llamador. SUM, FULL y FREQ permanecen disponibles; se
añadió `tspr3_radix` y se afinó `tspr3_auto`.

```powershell
.\scripts\build-legacy.ps1 -Million
python .\tools\analysis\summarize_million.py
```

El primer comando ejecuta las pruebas básicas y compila/ejecuta
`tspr3_million.c`. Sobrescribe los dos CSV de medición con una nueva ejecución.
Python es opcional. Los comandos se ejecutan desde la carpeta del proyecto.

Entorno verificado: Windows 11 Pro x64, AMD Ryzen 5 4600G (6 núcleos/12 hilos),
34.143.227.904 bytes de RAM, MSVC tools 14.44.35207, C17 `/O2 /W4 /WX`.
No se usa concurrencia, instrucciones exclusivas de AVX ni bibliotecas externas.
La compilación Linux alternativa sigue sin verificarse en esta entrega.

## Fórmulas aplicadas

Para n elementos y k valores distintos:

| Ruta | Selección | Tiempo y memoria auxiliar |
|---|---|---|
| Ascendente/descendente | Comparación exacta de vecinos | `O(n)`; `O(1)` |
| Ternas separadas | `max(T_i) <= min(T_(i+1))` | `O(n)`; `O(1)` |
| Tramos naturales | Hasta r = 64 tramos ascendentes o descendentes | `O(n log r)`; `O(n)` |
| Valor dominante | Conteo exacto d >= n-floor(n/10) | `O(n + p*m)` con m=n-d y p<=4; `O(m)` |
| Conteo directo | R=max-min+1, `R <= min(131072,floor(n/8))` | `O(n+R)`; `O(R)` |
| Frecuencias dispersas | `k <= B=min(floor(n/16),4096)` | Conteo hash acotado y ordenamiento de k claves |
| Radix | Resto de entradas grandes | `O(p*(n+256))`, p<=4; `O(n)` |

Los umbrales son políticas explícitas para acotar memoria y trabajo descartado;
no se presentan como constantes óptimas universales ni como resultados de una
nueva regresión. La regresión anterior modelaba dos algoritmos y no representa
las nuevas rutas lineales, por eso dejó de gobernar AUTO.

Radix convierte cada entero en una clave unsigned:

```text
key(x) = uint32(x) XOR 0x80000000
byte_j(x) = (key(x) >> (8*j)) AND 255, j=0,1,2,3
offset_j(b) = sum(count_j(t), t < b)
```

Cada pasada distribuye establemente por un byte, empezando por el menos
significativo. El XOR hace que el orden unsigned coincida con el orden signed,
incluidos `INT32_MIN` y `INT32_MAX`. Se cuentan todos los bytes en un recorrido
inicial y se omiten las pasadas cuyo byte es constante en toda la entrada.
Después de cada pasada, el prefijo de bytes procesados está ordenado; al terminar,
todos los bits de las claves quedan ordenados. No hay comparaciones de claves
en las pasadas y no se aplica la cota comparativa `n log n`.

Los tramos descendentes se invierten antes de fusionarlos. La fusión por pares
reduce el número de tramos geométricamente: `r_(j+1)=ceil(r_j/2)`, por lo que
necesita hasta `ceil(log2(r))` niveles. La tabla hash crece con
`capacity_(j+1)=2*capacity_j` y ocupación máxima del 50 %. AUTO abandona el conteo
al exceder B claves o alcanzar 64 posiciones ocupadas de sondeo; FREQ explícito
conserva su índice sin estos límites.

Para el valor dominante, 512 posiciones repartidas sugieren un candidato mediante
cancelación de votos. Se requiere al menos 461 coincidencias en la muestra y se
confirma después la condición del 90 % contando toda la entrada. Si se confirma,
se extraen y ordenan las m excepciones, se inserta el grupo de d repeticiones en
su posición y se conserva exactamente el multiconjunto original. Una muestra
engañosa solo puede añadir trabajo: no determina cuentas ni omite elementos.

Memoria auxiliar local con n=1.000.000: radix y fusión reservan aproximadamente
4 MB de buffer; conteo directo hasta 1 MiB de contadores en x64; índice AUTO
hasta unos 192 KiB de tabla e índice, sin contar posibles reservas internas de
`qsort`; dominante hasta unos 800 KB de buffers para las excepciones.
La salida ocupa 4 MB adicionales y pertenece al llamador. El benchmark reserva
también entrada y referencia; FREQ explícito puede utilizar bastante más memoria
que AUTO cuando casi todos los valores son distintos.

## Escenarios y corrección

Se ejecutaron 30 escenarios, cada uno con 1.000.000 de elementos y cinco semillas:

- Aleatoria en todo el dominio signed y aleatoria en un rango de dos millones.
- Ascendente, descendente, constante y permutación de valores distintos.
- Cardinalidades 8, 64, 512, 2048, 8192, 32768 y 131072.
- Repetidos dispersos con 8 y 2048 claves de rango amplio.
- Ternas separadas, valores intercalados, montaña y dientes de sierra.
- Casi ordenada con uno y cien intercambios, y 16 tramos ordenados.
- Extremos alternados, cuatro claves en los límites signed y bytes bajos constantes.
- Prefijo repetido, sufijo repetido, 99 % de ceros, un valor fuera de rango y grupos descendentes.

Resultados correctos en 750 ordenamientos: 150 entradas por cinco algoritmos,
comparadas elemento por elemento contra la referencia `qsort`. Para FREQ también
se comprobó la cantidad de valores distintos. SUM se verificó por suma, índice,
unicidad y desempate en las primeras semillas de los 30 escenarios.
Las pruebas básicas pasaron 5.033 casos, incluidos selección de rutas, límites
numéricos y una muestra intencionadamente engañosa. No se inyectaron fallas de
asignación ni se ejecutó un analizador de memoria.

## Medición y resultados

Cada semilla genera una entrada nueva; por tanto, la mediana resume cinco entradas
de la misma distribución. Las comparaciones se emparejan por semilla. El orden
de los cinco algoritmos rota entre escenarios y semillas. La generación y las
comprobaciones quedan fuera del tiempo; todos los algoritmos, incluido `qsort`,
incluyen la copia necesaria para conservar la entrada y sus reservas internas.
Se usa `QueryPerformanceCounter` en Windows. Se registran mediana, mínimo,
máximo y todas las observaciones; no hay pruebas de significancia ni intervalos
de confianza. No se fijó afinidad, energía ni carga del sistema.

Medianas finales, milisegundos:

| Escenario | AUTO | FULL | FREQ | RADIX | qsort |
|---|---:|---:|---:|---:|---:|
| Aleatoria en dominio completo | 12,176 | 80,238 | 208,653 | 11,428 | 103,313 |
| Ascendente | 1,596 | 14,830 | 213,381 | 11,888 | 29,771 |
| 8 valores distintos | 1,221 | 40,549 | 6,020 | 5,956 | 16,433 |
| 2048 valores distintos | 1,176 | 63,633 | 7,287 | 7,465 | 54,443 |
| 2048 valores dispersos | 7,016 | 62,784 | 7,137 | 11,249 | 54,724 |
| Ternas separadas | 2,137 | 13,496 | 206,128 | 11,329 | 31,069 |
| Intercalada | 12,557 | 24,410 | 207,731 | 12,023 | 70,256 |
| Montaña | 5,321 | 16,967 | 120,958 | 12,422 | 142,721 |
| Casi ordenada, un intercambio | 4,671 | 16,923 | 228,975 | 12,284 | 29,090 |
| 99 % de ceros | 1,998 | 23,636 | 4,491 | 13,647 | 29,808 |

Para cada entrada j, `speedup_j=T_FULL_j/T_AUTO_j`. La mediana de estos 150
cocientes fue **8,284**. AUTO fue más rápida que FULL en **150/150** mediciones
emparejadas. Esto describe esta batería y este equipo, no todas las entradas
posibles ni todos los equipos.

La selección aún cuesta tiempo: por ejemplo, radix directo puede ser más rápido
en datos de alta cardinalidad y FREQ directo en pocas claves dispersas. Respecto
al mejor algoritmo medido en cada entrada, el peor cociente individual
`T_AUTO/min(T_FULL,T_FREQ,T_RADIX,T_AUTO,T_qsort)` fue 1,678. Elegir entre rutas
no garantiza igualar un oráculo que conoce de antemano sus tiempos.

Archivos completos: `benchmarks/results/million_raw.csv` (750 tiempos individuales)
y `benchmarks/results/million_summary.csv` (150 resúmenes). `summarize_million.py`
reproduce los cocientes emparejados y los métodos elegidos.
Métodos AUTO: 0 FULL, 1 FREQ, 2 ascendente, 3 descendente, 4 ternas separadas,
5 radix, 6 conteo directo, 7 tramos naturales, 8 dominante.
