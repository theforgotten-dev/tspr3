# TSPR3

Biblioteca experimental de ordenamiento basada en ternas, frecuencias y un indice
binario de bloques. API C17, adaptador C++17, JAR Java 8 y puente JNI opcional.

## Empezar

Desde PowerShell en esta carpeta:

```powershell
.\build.ps1 -Task all
.\build.ps1 -Task maven
```

El primer comando compila y prueba C/C++, Java y JNI. El segundo genera los
artefactos Maven con fuentes y Javadoc, sin publicar. Requiere MSVC/CMake y un
JDK; la herramienta Maven se descarga localmente y se comprueba por SHA-512.

## Algoritmos

| Ruta | Trabajo |
|---|---|
| A | Ordenar ternas y fusionar usando sus extremos. |
| B | Compactar ternas y reagrupar multiplicidades durante la fusion. |
| C | Indexar ternas compactas por prefijos y resolver bloques cruzados. |
| AUTO | Elegir una estrategia mediante una politica separada. |

C usa la regresion documentada de costos; Java conserva su politica conservadora
hasta recalibrar pesos propios. C permanece experimental y explicita. El codigo
radix anterior se conserva en `experiments/legacy` y no se enlaza a la biblioteca.

## Estructura

```text
include/tspr3/       API C y adaptador C++
src/c/              Fachada, estrategias y politica C
src/c/internal/     Contratos privados y pesos compilados
java/               Modulo Maven, facade Java, estrategias y JNI
tests/c, tests/cpp/  Contratos de biblioteca y nucleo
examples/           Consumidores C y C++
benchmarks/c/       Generadores y programas de medicion
benchmarks/results/ Evidencia historica conservada
models/c/           Procedencia de pesos C
tools/analysis/     Ajuste y analisis reproducible
scripts/            Construccion, validacion y empaquetado
config/maven/       Plantilla sin credenciales
experiments/legacy/ Ensayos anteriores separados del producto
docs/               Arquitectura, contratos y publicacion
```

## Documentacion

- [Proyecto completo, actores y etapas de despliegue](docs/PROYECTO-Y-DESPLIEGUE.md).

- [Arquitectura: Facade, Strategy y Policy](docs/ARCHITECTURE.md).
- [Uso de la biblioteca C/C++](docs/LIBRARY.md).
- [Uso del JAR y JNI](java/README.md).
- [Preparacion de Maven Central](docs/PUBLISHING.md).
- [Validacion de la refactorizacion](docs/VALIDATION.md).
- [Propuesta original](docs/research/PROPOSAL.md).
- [Experimento de ternas e indice binario](docs/research/TRI-BIN.md).
- [Recorrido de dos millones e indicadores logaritmicos](docs/research/RUN-2M.md).

## Integracion

C/C++:

```cmake
find_package(TSPR3 0.1 CONFIG REQUIRED)
target_link_libraries(mi_programa PRIVATE TSPR3::tspr3)
```

Java:

```java
import org.tspr3.Tspr3;
int[] sorted = Tspr3.sort(input);
```

El JAR puede usarse directamente. Las coordenadas preparadas para Central son
`io.github.theforgotten-dev:tspr3:0.1.0`, pero todavia no estan publicadas.
El propietario confirmó el namespace verificado y la creación del token.
La clave pública está publicada y las firmas locales fueron verificadas.
Falta confirmar la publicación y resolución del artefacto desde Central.

## Licencia

Ambas bibliotecas se distribuyen bajo [Apache-2.0](LICENSE).
Copyright 2026 Juan Pablo Muñoz Leiva (theforgotten-dev).

## Alcance

La API actual ordena enteros de 32 bits y conserva la entrada. El siguiente paso
es definir claves, identidad y estabilidad para registros y estructuras. Una
misma clave no permite eliminar el payload de varios registros.

Las mejoras de tiempo dependen de la distribucion. Los resultados y limites
estan documentados; no se afirma novedad ni superioridad general del algoritmo.
