# Último recorrido: índice logarítmico, pesos y dos millones de elementos

## Qué se ejecutó

Se implementó un perfil logarítmico de ternas y un selector predictivo entre
A=TERNAS, B=TERNAS_FREQ y C=TERNAS_BIN. Las tres rutas conservan los bloques
triádicos y procesan exactamente todos los elementos. El índice logarítmico
se usa como información de selección, no como un nuevo algoritmo que reordene
individualmente los datos. La ruta C conserva el índice de prefijos de bloques.

Se mantuvieron los 30 escenarios anteriores y se añadieron seis distribuciones:
normal amplia, normal cuantizada, normal con proximidad dentro de las ternas,
lognormal, bimodal y normal con centros agrupados. Cada escenario tiene cinco
semillas y 2.000.000 de enteros. En las distribuciones locales se generan los
valores próximos directamente; no se transforma una entrada real para fabricar
un beneficio sin contabilizar su costo. No se presupone normalidad de entradas
reales ni se implementa una prueba formal de normalidad.

Los primeros tres seeds de cada escenario forman entrenamiento (108 entradas).
Los otros dos forman evaluación reservada (72 entradas). Los pesos se ajustan
exclusivamente con las filas de entrenamiento. La reserva mide nuevas semillas
de las mismas familias, no generalización a distribuciones desconocidas.

## Fórmula logarítmica y pesos

Se muestrean hasta 2048 ternas distribuidas por toda la secuencia, manteniendo
sus posiciones originales. Media y desviación estándar se calculan mediante
actualización incremental de Welford sobre sus valores. La escala s se limita
inferiormente a 1 para admitir entradas constantes.

```text
L(x) = signo(x-μ) * log2(1 + abs(x-μ)/s)
intervalo(T) = [L(min(T)), L(max(T))]
```

Nueve indicadores describen la muestra:

1. Repeticiones dentro de las ternas.
2. Repeticiones en todos los valores muestreados; no es una estimación exacta de k global.
3. Fracción de pares de ternas muestreadas con rangos separados.
4. Longitud media normalizada del prefijo binario común.
5. Proporción de ternas que cruzan la frontera de signo.
6. Anchura media del intervalo logarítmico.
7. Cruces de fronteras logarítmicas `[-2,-1,-0.5,0,0.5,1,2]`, normalizados.
8. Proporción ascendente entre mínimos muestreados.
9. Amplitud logarítmica total de los valores muestreados.

Para cada variante j, los indicadores x se estandarizan con medias c y escalas d
calculadas únicamente sobre entrenamiento:

```text
z_i = (x_i-c_i)/d_i
log(t_j / (n/1.000.000)) = w_j0 + sum(w_ji*z_i)
t_predicho_j = (n/1.000.000) * exp(w_j0 + sum(w_ji*z_i))
```

Los pesos se obtienen por regresión ridge, resolviendo:

```text
w_j = (XᵀX + λD)⁻¹ Xᵀy_j
λ = 0,01; D = diag(0,1,1,...,1)
```

No se penaliza el intercepto. Los pesos pueden ser negativos: describen
asociaciones predictivas, no precios positivos de instrucciones ni causalidad.
El ajuste en log tiempo garantiza predicciones positivas para n>0.
Para cada ruta se añade un margen usando el percentil 90 de errores positivos
en log tiempo de entrenamiento:

```text
e_j = percentil90(max(0, log(t_real/t_predicho)))
costo_conservador_j = t_predicho_j * exp(e_j)
```

Este margen no es un intervalo de confianza ni una garantía de tiempo máximo.
Se elige el menor costo conservador que cabe en el presupuesto de heap. A se
conserva si otra ruta no tiene al menos 10 % de ventaja prevista. C solo se
habilita si alguna entrada de entrenamiento demuestra una ventaja superior a
5 % frente a B. En este entrenamiento quedó deshabilitada: no demostró esa
ventaja. Se ajustó su modelo igualmente para documentar el error de predicción.

Pesos completos: `models/c/log_model.json` y `tspr3_log_weights.h`, generado
por `fit_log_selector.py`. El ajuste usa eliminación gaussiana con pivoteo y
Python estándar. Los umbrales 0,01, 10 %, 5 % y el percentil 90 son políticas
fijadas; no se optimizaron mirando las semillas reservadas.

## Ejecución y API

```powershell
.\scripts\build-log.ps1
python .\tools\analysis\fit_log_selector.py
.\scripts\validate-log.ps1 -Tests -Sanitize
.\scripts\validate-log.ps1
python .\tools\analysis\analyze_log_selector.py
```

`build-log.ps1` mide A/B/C y guarda las características de cada muestra.
El ajuste genera los pesos C; la validación ejecuta el selector sobre las dos
semillas reservadas por escenario. Los CSV de este recorrido tienen nombres
nuevos y preservan las mediciones anteriores.

API en `tspr3_log_selector.h`:

```c
tspr3_log_choice choice;
tspr3_tri_stats stats;
tspr3_status status = tspr3_log_sort(input, n, output,
    256 * 1024 * 1024, &choice, &stats);
/* Comprobar status; choice.selected informa A, B o C. */
```

La función incluye perfil, decisión y ordenamiento exacto. El límite es para
reservas auxiliares del heap; excluye entrada, salida del llamador y pila del
perfil. `tspr3_tri_memory_bound` calcula una cota conservadora con los tamaños
reales de los descriptores y nodos. Si ninguna ruta admisible cabe, devuelve
`TSPR3_NO_MEMORY`. La muestra ocupa aproximadamente 40 KiB de pila y no reserva
heap. La predicción no modifica los elementos ni reemplaza sus comparaciones.

MSVC C17 `/O2 /W4 /WX`, Ryzen 5 4600G, Windows x64. Se compiló sin advertencias.
Las pruebas de perfil, muestras finitas, presupuesto de memoria y argumentos
inválidos pasaron también con AddressSanitizer. Los tiempos se midieron sin
sanitizer. La nueva cota de memoria se comprobó contra las reservas reales en
todas las ejecuciones de validación.

## Resultados con 2.000.000 de elementos

540 ordenamientos A/B/C correctos (36 escenarios × 5 semillas × 3 variantes),
más 72 ordenamientos predictivos correctos. El `qsort` de la entrada completa
sirve como referencia de corrección, fuera del tiempo; no es una opción del
selector. El perfil también usa `qsort` sobre sus ternas y hasta 6144 valores
muestreados para medir repeticiones; ese costo sí se incluye en el selector.

Medianas de cinco semillas de la batería A/B/C, milisegundos:

| Escenario | A: ternas | B: frecuencias | C: binario |
|---|---:|---:|---:|
| Aleatoria signed | 178,797 | 240,171 | 315,579 |
| 8 valores distintos | 86,324 | 64,131 | 108,712 |
| 99 % de ceros | 57,457 | 32,571 | 85,139 |
| Normal amplia | 176,697 | 223,828 | 293,587 |
| Normal cuantizada | 97,301 | 69,116 | 118,776 |
| Normal con proximidad local | 101,300 | 152,723 | 830,118 |
| Lognormal | 119,208 | 103,133 | 241,378 |
| Bimodal | 151,906 | 155,352 | 203,111 |
| Normal con centros agrupados | 110,136 | 100,254 | 242,557 |

El selector eligió A en 45 entradas reservadas y B en 27. Acertó la variante
con el menor tiempo observado en 62/72 entradas (**86,1 %**). La mediana del
cociente `tiempo_elegido/min(t_A,t_B,t_C)` fue 1,000; el peor fue 1,149,
aproximadamente 14,9 % por encima de la mejor ruta observada. Estos cocientes
usan los tiempos emparejados de la batería A/B/C, no tiempos tomados en sesiones
distintas. No se presentan como exactitud poblacional ni significancia estadística.

Errores medianos relativos de predicción en las semillas reservadas:
A 15,92 %, B 15,17 %, C 17,58 %. La mediana del cociente A/ruta elegida es 1,000:
la selección no produce una aceleración general uniforme; evita varios escenarios
costosos y aprovecha beneficios puntuales de la compactación.

Los tiempos del selector completo, con perfil y decisión incluidos, se guardan
separadamente en `log_validation.csv`. Esa ejecución es posterior al ajuste y
no se mezcla con los tiempos emparejados usados para evaluar la elección.
Las mediciones incluyen instrumentación de comparaciones/escrituras, copia y
reservas, pero no generación ni verificación; el orden A/B/C rota. No se fijó
afinidad ni carga del sistema. Se conservan mínimos y máximos, sin intervalos
de confianza. No se midió la contribución independiente de cada indicador: el
86,1 % no demuestra que el logaritmo por sí solo sea responsable del resultado.

## Límites y evidencia

El modelo se entrenó y evaluó en n=2.000.000. La escala lineal que usa la fórmula
para otros tamaños no fue validada: no demuestra un exponente de complejidad ni
sustituye las cotas `O(n log n)` de las rutas. Una muestra adversa puede provocar
una selección desacertada, conservando siempre la corrección del ordenamiento.
El costo del perfil y la selección también cuenta; una política fija puede ganar
cuando ya se conoce la distribución.

En la batería, el perfil tuvo mediana 0,654 ms, mínimo 0,269 ms y máximo 1,425 ms.

La distribución normal no garantiza ternas de prefijos útiles. En la normal
local C redujo comparaciones numéricas, pero creó una gran cantidad de nodos y
tardó 830,118 ms; su pico auxiliar fue 168.080.392 bytes. No se induce una mejora
de tiempo simplemente aumentando la longitud del prefijo. Las rutas siguen
siendo experimentales y se documentan los resultados desfavorables.

Archivos:

- `benchmarks/results/tri_2m_raw.csv`: 540 tiempos, contadores, indicadores y partición entrenamiento/evaluación.
- `benchmarks/results/tri_2m_summary.csv`: 108 resúmenes por escenario y variante.
- `models/c/log_model.json`: medias, escalas, pesos y márgenes entrenados.
- `benchmarks/results/log_validation.csv`: 72 tiempos completos y elecciones reales del selector.
- `benchmarks/results/log_analysis.txt`: análisis reproducible de aciertos, cocientes y errores.
