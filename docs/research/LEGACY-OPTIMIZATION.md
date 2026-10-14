# Optimización aplicada y fundamento matemático

**Antecedente histórico:** este documento describe la primera selección AUTO
basada en regresión. La versión actual reemplaza ese presupuesto y añade radix,
conteo directo y tramos naturales. Implementación y resultados vigentes en
[MILLON.md](LEGACY-MILLION.md). Los datos originales siguen disponibles para reproducir
el ajuste; sus coeficientes ya no se usan en el código C.

## Modelo y ajuste

`n` es el número de elementos y `k` el número de valores distintos.
Se ajustaron 21 medianas locales (cinco repeticiones por configuración) de
entradas aleatorias y con duplicados, con n = 1.000, 10.000 y 100.000.
Se excluyeron las distribuciones ordenadas, de rangos separados e intercaladas
del ajuste: tienen estructura que este modelo no representa.

Modelos de tiempo, en milisegundos:

```text
T_FREQ(n,k) = a*n + b*k*log2(max(k,2))
T_FULL(n)   = c*n*log2(max(n,2))

a = 0.00000654393304238
b = 0.00000991487688323
c = 0.00000300746751816
```

La parte lineal representa conteo y reconstrucción; `k log2(k)` representa el
ordenamiento de claves distintas. Es un modelo empírico, no una garantía sobre
`qsort` ni sobre colisiones hash. El modelo FULL tampoco representa correctamente
todos los beneficios debidos a duplicados o estructura previa.

Para reproducir el ajuste (Python estándar, sin paquetes adicionales):

```powershell
python .\tools\analysis\analyze_benchmark.py benchmarks\results\calibration.csv
```

Con x = n, z = k*log2(max(k,2)), y = tiempo FREQ, mínimos cuadrados sin intercepto:

```text
D = sum(x²)*sum(z²) - sum(x*z)²
a = [sum(x*y)*sum(z²) - sum(z*y)*sum(x*z)] / D
b = [sum(z*y)*sum(x²) - sum(x*y)*sum(x*z)] / D

u = n*log2(max(n,2))
c = sum(u*tiempo_FULL) / sum(u²)
```

Error del ajuste: `RMSE = sqrt(sum((observado - predicho)²)/N)`.
RMSE en calibración: FREQ 0,077064 ms; FULL 0,675449 ms. Son errores de ajuste
en los mismos datos; no son intervalos de confianza ni validación independiente.

## Aplicación en C

Se usa FREQ solo si el conteo exacto final no supera el mayor entero B que cumple:

```text
a*n + b*B*log2(max(B,2)) <= 0.65*c*n*log2(n)
B <= floor(n/4)
```

`0,65` es un margen conservador elegido, no un coeficiente ajustado. El límite
`n/4` acota el presupuesto de claves; no significa que la memoria total sean
`n/4` bytes. La tabla crece geométricamente: `capacity_(j+1) = 2*capacity_j`,
manteniendo ocupación máxima del 50%. La búsqueda binaria de B cuesta `O(log n)`;
la monotonicidad de `B log2(max(B,2))` permite descartar mitades del intervalo.

Al aparecer la clave B+1 se abandona el índice parcial y se ejecuta FULL.
No se procesa dos veces un índice completo cuando se selecciona FREQ. Este
presupuesto limita claves distintas, pero un prefijo largo repetido puede requerir
mucho conteo antes de descubrir que la entrada excede B. No se promete mejorar
todas las distribuciones ni el peor caso adverso de la tabla hash.

También se aplicaron dos funciones exactas de estructura:

- Entrada monótona: copia o inversión en tiempo lineal.
- Ternas con rangos ascendentes disjuntos: `max(T_i) <= min(T_(i+1))`; se ordena cada terna en tiempo lineal y se omite toda fusión.

## Validación después del ajuste

Compilación MSVC C17 `/O2 /W4 /WX`, sin advertencias. Pasaron 16 casos
deterministas y 5.010 aleatorios, incluyendo todos los métodos, límites del
presupuesto y un prefijo repetido seguido por claves distintas.
Se comprobaron las salidas FULL/FREQ/AUTO contra `qsort` en 66 configuraciones,
cinco repeticiones cada una. Los tamaños 3.000, 30.000 y 200.000 no participaron
en el ajuste; constituyen validación posterior de rendimiento y corrección,
no una validación estadística exhaustiva del modelo.

Ejemplos en n = 200.000, mediana en milisegundos:

| Distribución | FULL | AUTO | qsort |
|---|---:|---:|---:|
| 8 valores distintos | 6,343 | 1,007 | 3,363 |
| Ascendente | 0,907 | 0,214 | 4,533 |
| Rangos separados por terna | 0,914 | 0,365 | 5,206 |
| Aleatoria, 190.232 valores distintos | 13,314 | 16,944 | 19,668 |
| Intercalada | 3,534 | 7,047 | 16,685 |

AUTO mejora duplicados y rangos ordenados; el intento de conteo encarece los
casos con alta cardinalidad, especialmente los intercalados. FULL y FREQ siguen
disponibles para quien conoce la distribución. No se presenta AUTO como ganador
universal. Medición en orden fijo, sin análisis de dispersión ni control de carga
del sistema: conviene recalibrar en otro equipo antes de adoptar sus umbrales.

Datos conservados: `benchmarks/results/calibration.csv` y `benchmarks/results/optimization.csv`.
Ambos archivos tienen estas columnas: distribución,n,k,método,FULL,FREQ,AUTO,qsort.
La calibración registra el AUTO preliminar de presupuesto fijo. Solo FULL y FREQ
se usan en el ajuste; los tiempos están expresados en milisegundos.
Métodos: 0 FULL, 1 FREQ, 2 ascendente, 3 descendente, 4 ternas separadas.

Regenerar el benchmark sin sobrescribir la calibración usada:

```powershell
.\build.ps1 -Task legacy -Benchmark | Tee-Object -FilePath build\optimization.txt
```
