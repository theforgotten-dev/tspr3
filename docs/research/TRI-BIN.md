# TSPR-3-BIN: implementación y resultado del experimento

Esta entrega implementa la propuesta de ternas, frecuencias y prefijos de bloques.
Las tres variantes producen la misma lista completa ordenada. Se comparan entre
sí para medir el aporte de cada mecanismo; `qsort` se usa exclusivamente para
verificar la salida, fuera del tiempo medido.

## Variantes ejecutadas

| Variante | Procedimiento |
|---|---|
| TERNAS | Ordenar cada terna y fusionar bloques por pares con pruebas de extremos. |
| TERNAS_FREQ | Compactar cada terna como `(valor,cantidad)` y sumar cantidades iguales durante las mismas fusiones por pares. |
| TERNAS_BIN | Construir las mismas ternas compactas; indexar bloques completos por prefijo común; resolver grupos y fusionar los bloques que cruzan fronteras. |

El ejecutable enlaza solamente `tspr3_tri.c` y el programa correspondiente de
pruebas o benchmark. La implementación de AUTO/RADIX anterior queda como
antecedente en archivos independientes y no participa en este experimento.
Los generadores de entrada se comparten mediante `tspr3_scenarios.h` para
conservar exactamente las distribuciones y semillas anteriores.

## Uso

```powershell
.\scripts\build-tri.ps1
.\scripts\build-tri.ps1 -Million
.\scripts\build-tri.ps1 -Sanitize
python .\tools\analysis\summarize_tri.py
```

Requisitos: herramientas MSVC C/C++ x64 de Visual Studio 2022 y PowerShell.
`-Sanitize` usa AddressSanitizer de MSVC para las pruebas pequeñas. No se usó
sanitizer en la medición de tiempos. Python es opcional para analizar CSV.

Compilación GCC/Clang preparada, no ejecutada en este equipo:

```bash
cc -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror tspr3_tri.c tspr3_tri_test.c -o tri_test
./tri_test
cc -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror tspr3_tri.c tspr3_tri_bench.c -o tri_bench
mkdir -p measurements
./tri_bench
```

API en `tspr3_tri.h`:

```c
int32_t input[] = {9,8,9, 15,12,13, 8,8,9};
int32_t output[9];
tspr3_tri_stats stats;
tspr3_status status = tspr3_tri_sort(input, 9, TSPR3_TRI_BIN, output, &stats);
/* Comprobar status antes de usar output. */
```

`tspr3_tri_index` devuelve directamente la salida compacta para COUNTS o BIN:
`[(8,3),(9,3),(12,1),(13,1),(15,1)]` en el ejemplo. El llamador libera el índice
con `free`. Su arena tiene capacidad para n registros; la cantidad devuelta k
indica cuántos están ocupados. `tspr3_tri_sort` también mide la expansión final
al producir sus n enteros. La entrada permanece intacta y no debe solaparse con
la salida. Con n=0 se admiten punteros de datos nulos.

## Fórmulas y estructura del índice

Los enteros ya están en representación binaria; se operan directamente sus bits.
Para mantener el orden signed:

```text
key(x) = uint32(x) XOR 0x80000000
diferencia(T) = key(min(T)) XOR key(max(T))
bits_prefijo(T) = ceros_iniciales(diferencia(T))
prefijo(T) = primeros bits_prefijo(T) bits de key(min(T))
```

Si mínimo=máximo, el prefijo tiene 32 bits. Una diferencia en el bit de signo
produce prefijo de longitud cero. El bit de signo solo cambia en la clave del
índice, conservando los valores de la terna. Mínimo y máximo se obtienen de la
terna ordenada. SUM permanece disponible separadamente; sus sumas no se usan
como claves para ordenar elementos completos.

Se usa un árbol de prefijos comprimido: cada nodo almacena su prefijo, dos hijos
y una lista de ternas. Solo se crean nodos para grupos existentes y puntos de
ramificación. El índice almacena referencias de bloques; no redistribuye los
elementos individuales de una terna por sus dígitos.

Una terna se conserva en el nodo cuyo prefijo común cubre todo su intervalo.
Si cruza la separación entre hijos, pertenece al padre. Los hijos izquierdo y
derecho ocupan intervalos numéricos disjuntos y ordenados, por lo que sus salidas
pueden concatenarse directamente cuando el padre no tiene bloques propios.
Los bloques propios del padre se fusionan con los hijos; no se supone que el
padre preceda o suceda a sus descendientes.

Dentro de cada grupo, las ternas se fusionan por pares. Las pruebas de extremos
permiten concatenar rangos estrictamente separados. Cuando las claves coinciden,
se emite un registro y se suman sus cuentas. Usar separación estricta en la
salida compacta garantiza que las igualdades entre extremos también se reagrupen.

## Corrección y costo

Cada terna compacta conserva su multiconjunto. Una fusión emite claves ordenadas
y conserva todas las cuentas; al unir claves iguales solo combina multiplicidades.
El índice conserva cada bloque exactamente una vez. Por inducción en el árbol,
resolver un nodo produce la unión ordenada de sus bloques y descendientes.
Expandir cada cuenta reconstruye la lista completa con la misma longitud y
multiconjunto que la entrada.

Con m=ceil(n/3), la base y la compactación tienen costo peor `O(n log m)`;
los duplicados pueden reducir considerablemente el tamaño de los bloques
compactos posteriores. Los niveles de fusión siguen la progresión
`m_(j+1)=ceil(m_j/2)`.

El índice binario tiene longitud máxima de prefijo 32 y hasta `2*m+1` nodos.
La inserción visita como máximo una cadena de prefijos estrictamente crecientes.
La resolución añade trabajo de indexación, reordenamiento de bloques y fusión
de grupos padres; su cota conservadora es `O(n log m + 32*n)` para este dominio.
No garantiza reducir comparaciones: puede cambiar desfavorablemente el orden de
las fusiones. La memoria auxiliar de las tres variantes es `O(n)`, con constantes
distintas: el registro `(int32_t,size_t)` ocupa 16 bytes en este x64, mientras que
un entero ocupa 4. BIN reserva además descriptores y nodos.

## Verificación ejecutada

MSVC 14.44.35207, C17, `/O2 /W4 /WX`, Ryzen 5 4600G, Windows x64.

- 90.399 entradas pequeñas: enumeración exhaustiva de longitudes 0..8 sobre cuatro valores, aleatorios, extremos signed, ordenados y repetidos.
- En cada entrada se verificaron las tres salidas completas y ambas salidas compactas contra una referencia independiente.
- Las mismas pruebas pasaron con AddressSanitizer, sin errores detectados.
- 30 distribuciones con cinco semillas y 1.000.000 de elementos por entrada: 450 salidas completas correctas y cantidades de valores distintos correctas para las variantes compactas.

No se inyectaron fallas de asignación. AddressSanitizer se ejecutó sobre las
pruebas pequeñas, no sobre los 450 ordenamientos de un millón.

## Resultados de tiempo

Medianas de cinco semillas, milisegundos; instrumentación habilitada en las tres
variantes. Generación, referencia y verificaciones quedan fuera del tiempo.
Se incluyen todas las reservas de la implementación y la expansión de frecuencias.
El orden de las variantes rota entre semillas y distribuciones.

| Distribución | TERNAS | TERNAS_FREQ | TERNAS_BIN |
|---|---:|---:|---:|
| Aleatoria en dominio completo | 83,320 | 105,613 | 128,923 |
| Ascendente | 13,961 | 57,949 | 83,468 |
| Todos iguales | 15,102 | 14,698 | 17,663 |
| 8 valores distintos | 45,434 | 31,784 | 51,260 |
| 64 valores distintos | 55,593 | 43,146 | 63,344 |
| 2048 valores distintos | 67,189 | 65,410 | 85,127 |
| 8 valores dispersos | 41,392 | 31,664 | 50,350 |
| Ternas separadas | 14,720 | 57,842 | 83,330 |
| Extremos alternados | 32,061 | 17,535 | 22,015 |
| 99 % de ceros | 29,856 | 15,965 | 39,296 |

TERNAS_FREQ fue más rápida que la base en 66/150 entradas emparejadas.
TERNAS_BIN fue más lenta que TERNAS_FREQ en las 150 entradas. Esta implementación
no demuestra una mejora de tiempo debida al índice binario.

## Comparaciones, movimiento y memoria

Ejemplo con ocho valores distintos, medianas:

| Métrica | TERNAS | TERNAS_FREQ | TERNAS_BIN |
|---|---:|---:|---:|
| Comparaciones numéricas lógicas | 18.889.722 | 3.970.778 | 3.894.701 |
| Bytes escritos en buffers de datos | 88.000.000 | 49.774.976 | 60.910.600 |
| Pico de reservas auxiliares, bytes | 4.000.000 | 40.000.016 | 40.000.912 |

La compactación reduce comparaciones y gana tiempo en este caso. El índice reduce
un poco más las comparaciones numéricas, pero sus visitas, clasificación y copia
de bloques cuestan más que el ahorro. En entrada aleatoria signed, aproximadamente
250.047 de 333.334 ternas cruzan la frontera de signo y quedan en la raíz; por eso
el índice no logra separar la mayor parte del trabajo.

En entrada ascendente, BIN creó 416.668 nodos, con pico auxiliar 84.040.208 bytes
y 608.326.176 bytes escritos en buffers de datos. El índice resulta costoso incluso
cuando los bloques admiten prefijos específicos y rangos separados.

Los contadores representan operaciones lógicas sobre claves y escrituras de
registros/enteros persistentes; no son instrucciones de CPU ni tráfico físico
de memoria. No incluyen escrituras de metadatos ni variables temporales de pila.
Los bytes de salida compacta se cuentan por registro emitido; las cuentas locales
incrementadas se contabilizan adicionalmente. El pico auxiliar mide bytes pedidos
al asignador, incluyendo arenas y crecimiento simultáneo del índice; no es RSS.
La entrada y la salida completa del llamador se excluyen. Las clasificaciones y
visitas de prefijo se registran separadamente de las comparaciones numéricas.

Datos completos: `benchmarks/results/tri_raw.csv`, `benchmarks/results/tri_summary.csv` y
`benchmarks/results/tri_analysis.txt`. La estadística se limita a medianas, mínimos,
máximos y cocientes emparejados; no se ajustó una nueva regresión ni se presentan
intervalos de confianza. Los tiempos de esta batería instrumentada no se mezclan
con los benchmarks históricos de otras implementaciones.
