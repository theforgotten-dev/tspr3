# TSPR-3: propuesta experimental de partición triádica

**Estado:** hipótesis de investigación, no algoritmo novedoso ni mejora de rendimiento demostrados.

## 1. Motivación

Explorar si agrupar una secuencia en ternas y calcular propiedades locales (suma, mínimo y máximo) permite organizar datos y reducir trabajo en fases posteriores, especialmente en escenarios paralelos.

**Distinción esencial:** ordenar las sumas de ternas **no equivale** a ordenar globalmente los elementos originales. Son dos problemas diferentes y deben medirse por separado.

## 2. Definición formal

Sea una secuencia de entrada `A = [a0, a1, ..., a(n-1)]`. Formamos bloques consecutivos de hasta tres elementos:

`T_i = A[3i : min(3i+3, n)]`, para `i = 0, ..., ceil(n/3)-1`.

Para cada bloque calculamos:

- `s_i = sum(T_i)`
- `lo_i = min(T_i)`
- `hi_i = max(T_i)`
- `count_i = len(T_i)`

Si el bloque tiene exactamente tres valores y la aritmética es exacta, el valor central se puede obtener como `mid_i = s_i - lo_i - hi_i`. Con enteros de ancho fijo se debe controlar el desbordamiento; con coma flotante hay que considerar redondeo y valores especiales.

## 3. Dos variantes para no confundir objetivos

### Variante A: TSPR-3-SUM (idea inicial)

1. Dividir la entrada en ternas.
2. Calcular la suma de cada terna, potencialmente en paralelo.
3. Ordenar **las sumas**, manteniendo el índice original de la terna para trazabilidad.
4. Entregar los registros `(suma, indice_original)` ordenados.

Ejemplo:

```text
A = [9,2,7, 12,4,1, 8,5,3, 6,11,10]
T = [[9,2,7], [12,4,1], [8,5,3], [6,11,10]]
sumas = [18,17,16,27]
resultado = [(16,2), (17,1), (18,0), (27,3)]
```

La complejidad de calcular las sumas es `O(n)` y ordenar las `m = ceil(n/3)` sumas mediante un algoritmo comparativo es `O(m log m)` en el peor caso si se elige uno con esa garantía. En conjunto sigue siendo `O(n log n)` asintóticamente. No se ha demostrado una mejora frente a ordenar los `n` elementos porque la salida y el problema son distintos.

### Variante B: TSPR-3-FULL (extensión experimental)

Objetivo distinto: producir todos los elementos de `A` en orden ascendente.

1. Ordenar internamente cada terna (hasta tres comparaciones por bloque de tres elementos).
2. Fusionar bloques ordenados recursivamente.
3. Antes de fusionar dos bloques ordenados `L` y `R`, comprobar:
   - Si `max(L) <= min(R)`, concatenar `L + R`.
   - Si `max(R) <= min(L)`, concatenar `R + L`.
   - En otro caso, hacer una fusión ordenada estándar.
4. Explorar ejecución paralela **solo** para bloques suficientemente grandes y con límites explícitos de concurrencia.

**Cuidado:** la regla de extremos evita comparar elementos durante la fusión, pero copiar/concatenar bloques sigue teniendo costo `O(|L|+|R|)` en una implementación con arreglos nuevos. Para obtener una mejora real habría que estudiar vistas, referencias a segmentos o un diseño que reduzca movimiento de memoria. La variante completa se parece a técnicas conocidas de merge sort y detección de rangos; su novedad no está establecida.

## 4. Hipótesis comprobables

- **H1:** calcular resúmenes de ternas aporta información útil para organizar o filtrar bloques en determinados conjuntos de datos.
- **H2:** en datos con rangos de bloques ya separados, la prueba `max/min` reduce comparaciones de fusión.
- **H3:** la reducción de comparaciones compensa, o no, los costos de memoria y verificaciones adicionales.
- **H4:** el paralelismo solo acelera para tamaños superiores a un umbral medible y con recursos acotados.

No asumir que H1-H4 son verdaderas antes de experimentar.

## 5. Casos de prueba obligatorios

| Caso | Entrada | Resultado esperado SUM | Resultado esperado FULL |
|---|---|---|---|
| Ejemplo base | `[9,2,7,12,4,1,8,5,3,6,11,10]` | sumas `[16,17,18,27]` | `[1,2,3,4,5,6,7,8,9,10,11,12]` |
| Vacío | `[]` | `[]` | `[]` |
| Menos de tres | `[5,1]` | `[6]` | `[1,5]` |
| No múltiplo de tres | `[4,2,9,1]` | `[1,15]` | `[1,2,4,9]` |
| Negativos | `[-5,2,1,0,-2,4]` | `[-2,2]` | `[-5,-2,0,1,2,4]` |
| Repetidos | `[3,3,3,3,3,3]` | `[9,9]` | `[3,3,3,3,3,3]` |

Para SUM, verificar también que cada suma mantiene su índice de terna original. En empates, definir orden estable por `(suma, indice_original)`.

## 6. Protocolo de verificación local

### Entorno sugerido

Linux/WSL con Python 3.10+ para un prototipo verificable. Después, si hay señales prometedoras, repetir en C o Java con mediciones controladas. No es necesario Docker, servicios cloud ni agentes autónomos.

### Implementación mínima de referencia

Guardar el siguiente código como `tspr3_verify.py`:

```python
from random import Random
from time import perf_counter_ns


def ternas(a):
    return [a[i:i + 3] for i in range(0, len(a), 3)]


def tspr3_sum(a):
    return sorted(
        ((sum(t), i) for i, t in enumerate(ternas(a))),
        key=lambda x: (x[0], x[1]),
    )


def merge(left, right):
    if not left:
        return right
    if not right:
        return left
    if left[-1] <= right[0]:
        return left + right
    if right[-1] <= left[0]:
        return right + left
    out = []
    i = j = 0
    while i < len(left) and j < len(right):
        if left[i] <= right[j]:
            out.append(left[i]); i += 1
        else:
            out.append(right[j]); j += 1
    out.extend(left[i:])
    out.extend(right[j:])
    return out


def tspr3_full(a):
    blocks = [sorted(t) for t in ternas(a)]
    while len(blocks) > 1:
        nxt = []
        for i in range(0, len(blocks), 2):
            if i + 1 < len(blocks):
                nxt.append(merge(blocks[i], blocks[i + 1]))
            else:
                nxt.append(blocks[i])
        blocks = nxt
    return blocks[0] if blocks else []


def verify():
    rng = Random(20261013)
    for n in range(0, 501):
        for _ in range(10):
            a = [rng.randint(-1000, 1000) for _ in range(n)]
            expected_sum = sorted(
                ((sum(a[i:i + 3]), i // 3)
                 for i in range(0, n, 3)),
                key=lambda x: (x[0], x[1]),
            )
            assert tspr3_sum(a) == expected_sum, ("SUM", a)
            assert tspr3_full(a) == sorted(a), ("FULL", a)
    print("OK: 5010 casos aleatorios para ambas variantes")


def bench():
    rng = Random(42)
    for n in (1000, 10000, 100000):
        a = [rng.randint(-10**6, 10**6) for _ in range(n)]
        for label, fn in (
            ("SUM", tspr3_sum),
            ("FULL", tspr3_full),
            ("sorted(A)", sorted),
        ):
            samples = []
            for _ in range(5):
                t0 = perf_counter_ns()
                fn(a)
                samples.append((perf_counter_ns() - t0) / 1e6)
            samples.sort()
            print(f"n={n:>6} {label:>9}: mediana={samples[2]:.3f} ms")
        print("Nota: SUM y FULL no producen el mismo tipo de salida")


if __name__ == "__main__":
    verify()
    bench()
```

Ejecutar:

```bash
python3 --version
python3 tspr3_verify.py
```

En PowerShell:

```powershell
py -3 --version
py -3 .\experiments\legacy\python\tspr3_verify.py
```

### Cómo interpretar el benchmark

- `sorted(A)` es una referencia de ordenamiento **completo** comparable con FULL, pero está implementada en código nativo optimizado; la comparación con un prototipo Python no demuestra superioridad o inferioridad intrínseca del algoritmo.
- SUM resuelve otro problema. Para comparar SUM justamente, usar como baseline `sorted((sum(t), i) for i, t in enumerate(ternas(a)))` y verificar salidas idénticas.
- Repetir en varias distribuciones: aleatoria, ascendente, descendente, duplicados, bloques de rangos disjuntos y bloques intercalados.
- Registrar CPU, RAM, versión del compilador/intérprete, tamaño de entrada, repeticiones, mediana, dispersión, asignaciones de memoria y número de comparaciones.
- No paralelizar hasta obtener una línea base secuencial correcta y perfilada.

## 7. Criterios para avanzar

1. **Corrección:** todas las pruebas deterministas y aleatorias pasan.
2. **Definición:** se decide si el objetivo real es SUM, FULL u otra transformación basada en ternas.
3. **Medición:** se detecta un beneficio reproducible bajo alguna distribución concreta, no solo un resultado aislado.
4. **Formalización:** se documentan invariantes, prueba de corrección y complejidad temporal/espacial.
5. **Originalidad:** se revisa literatura antes de afirmar novedad, distinguiendo una idea propia de una técnica previamente conocida.

## 8. Seguridad experimental

El script usa tamaños acotados (`n <= 100000` en benchmark), cinco repeticiones por configuración y ejecución local sin red. No inicia procesos adicionales, hilos, escrituras periódicas ni servicios de pago. Para pruebas mayores, incrementar tamaños gradualmente y vigilar memoria/CPU.

---

**Pregunta de investigación central:** ¿qué propiedades útiles de una terna permiten reducir trabajo posterior, y en qué distribuciones ese ahorro supera el costo de calcular y mantener dichas propiedades?
