# Biblioteca TSPR3 0.1.0 para C y C++

Fuentes actuales en `src/c`, contratos privados en `src/c/internal` y API pública
en `include/tspr3`. Arquitectura: [ARCHITECTURE.md](ARCHITECTURE.md).

Biblioteca estática C17 con API pública C y adaptador C++17. Implementa el
selector logarítmico y las tres rutas basadas en ternas: A, B y C.
Entrada actual: enteros signed de 32 bits. La generalización a registros y
estructuras de datos corresponde a la siguiente etapa.

## Compilar, probar y obtener el paquete

En Windows, con Visual Studio 2022, herramientas C/C++ x64 y CMake:

```powershell
.\scripts\build-library.ps1
```

El script crea `build/library/Release/tspr3.lib`, ejecuta pruebas de API en C
y C++, ejecuta los ejemplos, instala el paquete en `dist/tspr3`, comprueba
consumidores de ese paquete y crea `dist/tspr3-windows-x64-0.1.0.zip`.
La compilación verificada es Release x64 con el runtime dinámico de MSVC (`/MD`).
Usar un runtime compatible al enlazar manualmente. Python no es necesario
para compilar ni para usar la biblioteca; los pesos ya están incorporados.

Alternativa CMake portátil preparada, todavía no verificada en Linux:

```bash
cmake -S . -B build/library -DCMAKE_BUILD_TYPE=Release
cmake --build build/library
ctest --test-dir build/library --output-on-failure
cmake --install build/library --prefix dist/tspr3
```

En otros sistemas el archivo estático suele llamarse `libtspr3.a` y CMake
propaga el enlace con la biblioteca matemática. Los benchmarks y el código
histórico de otros algoritmos no forman parte del target `TSPR3::tspr3`.

## Consumir mediante CMake

```cmake
find_package(TSPR3 0.1 CONFIG REQUIRED)
target_link_libraries(mi_programa PRIVATE TSPR3::tspr3)
# Si se usa el adaptador C++:
target_compile_features(mi_programa PRIVATE cxx_std_17)
```

Configurar el consumidor con `-DCMAKE_PREFIX_PATH=/ruta/al/paquete/tspr3`.
Los únicos headers públicos instalados son `tspr3/tspr3.h` y `tspr3/tspr3.hpp`.
No es necesario incluir los headers internos del proyecto.

## Uso desde C

```c
#include <tspr3/tspr3.h>

int32_t input[] = {9, 8, 9, 15, 12, 13, 8, 8, 9};
int32_t output[9];
tspr3_options options;
tspr3_report report;
tspr3_options_init(&options);

tspr3_result status = tspr3_sort_i32(input, 9, output, &options, &report);
/* Comprobar status == TSPR3_SUCCESS antes de usar output. */
```

Para forzar una ruta: `options.algorithm = TSPR3_ALGORITHM_A`, `_B` o `_C`.
El valor predeterminado es `TSPR3_ALGORITHM_AUTO` y un límite auxiliar de 256 MiB.
Pasar NULL como options aplica esos valores; el report es opcional.

`tspr3_counts_i32` devuelve `(value,count)` ordenados y únicos. El llamador
reserva n registros `tspr3_frequency_i32`; la función informa cuántos se usaron.
En esa interfaz AUTO significa B, C se puede solicitar explícitamente y A es
inválida, porque A no produce una salida compacta. Así, el selector de salida
completa no se confunde con una transformación de frecuencia distinta.

Funciones públicas:

| Función | Uso |
|---|---|
| `tspr3_sort_i32` | Lista completa ordenada mediante AUTO o A/B/C. |
| `tspr3_counts_i32` | Salida compacta mediante B/C. |
| `tspr3_options_init` | Configuración predeterminada. |
| `tspr3_version` | Versión de la API empaquetada. |
| `tspr3_result_message` | Descripción del estado devuelto. |

## Uso desde C++17

```cpp
#include <tspr3/tspr3.hpp>

std::vector<int32_t> input{9, 8, 9, 15, 12, 13, 8, 8, 9};
auto ordered = tspr3::sort(input);
auto grouped = tspr3::frequencies(input);
// ordered.values: 8,8,8,9,9,9,12,13,15
// grouped.values: (8,3),(9,3),(12,1),(13,1),(15,1)
```

También se admiten un puntero y una longitud. Los resultados incluyen un vector
y `diagnostics`. El adaptador usa la misma biblioteca C; no vuelve a implementar
el algoritmo. Traduce argumentos inválidos a `std::invalid_argument`, límite de
heap a `std::length_error` y falla de reserva a `std::bad_alloc`. Las reservas
de los vectores pueden lanzar sus propias excepciones antes de llamar a C.

## Contratos, diagnóstico y límites

- La entrada se conserva; entrada y salida deben ocupar regiones distintas.
- El llamador garantiza la capacidad real de los buffers y la validez de sus punteros.
- Con n=0 se admiten punteros de datos nulos. El contador de salida de `counts` es obligatorio.
- C devuelve `TSPR3_SUCCESS`, `TSPR3_INVALID_ARGUMENT`, `TSPR3_ALLOCATION_FAILURE` o `TSPR3_HEAP_LIMIT`.
- Las reservas temporales se crean y liberan dentro de la biblioteca; no se entrega memoria que el consumidor deba liberar con otro runtime.
- No se promete preservar la salida ante cualquier error; consumirla únicamente cuando el estado sea SUCCESS.
- Las llamadas usan estado local y pesos constantes; pueden ejecutarse en paralelo con buffers y reportes independientes. La biblioteca no crea hilos.
- El límite se aplica a una cota conservadora de heap auxiliar; excluye entrada, salida del llamador y unos 40 KiB de pila del perfil. Puede rechazar una ruta aunque sus reservas efectivas hubieran cabido.
- En C++, el límite no incluye los vectores de resultado. Sus capacidades y construcción también cuestan memoria y tiempo.

El report expone la ruta elegida, predicciones para A/B/C en AUTO, comparaciones,
escrituras lógicas y pico auxiliar. En rutas explícitas las predicciones son cero.
El contador de valores distintos es cero en A, que no los calcula. Estos contadores
no representan instrucciones de CPU ni memoria residente del proceso.

Los pesos están calibrados en este equipo para las distribuciones de dos millones
de elementos documentadas en `RECORRIDO-2M.md`. AUTO sigue produciendo resultados
exactos para otros tamaños, pero sus estimaciones de tiempo no están validadas allí.
C está deshabilitada en el selector entrenado por falta de ventaja y permanece
disponible como elección explícita experimental. No se promete superioridad
uniforme de AUTO ni estabilidad para futuros registros con identidad asociada.

La versión 0.1.0 es experimental. Se debe recompilar el consumidor al cambiar de
versión de API; no se ofrece todavía una garantía de ABI entre versiones menores.

## Validación de esta entrega

Se verificaron la compilación C17 y el enlace desde C++17, las cuatro elecciones,
frecuencias, negativos y extremos, entrada inmutable, buffers vacíos y errores.
También se comprobó un proyecto separado que encuentra el paquete instalado con
`find_package` y ejecuta ambos ejemplos. La validación anterior del núcleo con
dos millones permanece en `RECORRIDO-2M.md`; empaquetar no implica repetir sus
tiempos ni atribuirlos a los adaptadores.

## Siguiente etapa: estructuras de datos

El próximo diseño deberá definir extracción de clave, conservación de cada
registro y estabilidad. Dos registros con la misma clave no son necesariamente
duplicados completos: compactarlos solo en una cuenta podría perder información.
Una API de permutaciones de índices permitiría conservar el contenido original
y medir por separado el movimiento de estructuras. Esta entrega no introduce
todavía esa API ni comparadores genéricos.
