# TSPR3

[![Library contracts](https://github.com/theforgotten-dev/tspr3/actions/workflows/ci.yml/badge.svg)](https://github.com/theforgotten-dev/tspr3/actions/workflows/ci.yml)
[![CodeQL](https://github.com/theforgotten-dev/tspr3/actions/workflows/codeql.yml/badge.svg)](https://github.com/theforgotten-dev/tspr3/actions/workflows/codeql.yml)
[![Maven Central](https://img.shields.io/maven-central/v/io.github.theforgotten-dev/tspr3)](https://central.sonatype.com/artifact/io.github.theforgotten-dev/tspr3)

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

- [Todos los comandos PowerShell del proyecto](docs/POWERSHELL.md).

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

La version `io.github.theforgotten-dev:tspr3:0.1.1` esta publicada en Maven
Central y su POM publico fue verificado el 3 de noviembre de 2026.
El paquete Java sigue siendo `org.tspr3`, independiente del groupId.
Consulta [el flujo de publicacion](docs/PUBLISHING.md).

## Pipeline local

Webhook autenticado, incremento de version, validacion y publicacion con
sincronizacion atomica de commit/tag: [guia del pipeline](docs/LOCAL-PIPELINE.md).

Consulta tambien [CHANGELOG](CHANGELOG.md), [contribucion](CONTRIBUTING.md),
[seguridad](SECURITY.md) y [releases](RELEASING.md).

## Licencia

Ambas bibliotecas se distribuyen bajo [Apache-2.0](LICENSE).
Copyright 2026 Juan Pablo Muñoz Leiva (theforgotten-dev).

## Alcance

La API publica actual ordena enteros de 32 bits y conserva la entrada. El motor
de registros genericos es interno y experimental; no forma parte de la ABI.

Las mejoras de tiempo dependen de la distribucion. Los resultados y limites
estan documentados; no se afirma novedad ni superioridad general del algoritmo.
