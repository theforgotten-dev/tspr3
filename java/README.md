# TSPR3 Java 0.1.0

La API mantiene compatibilidad tras la organización del repositorio. La fachada,
las estrategias y AUTO están separadas; diseño en [ARCHITECTURE.md](../docs/ARCHITECTURE.md).
La construcción Maven y sus requisitos están en [PUBLISHING.md](../docs/PUBLISHING.md).
Los tiempos siguientes son evidencia previa a la refactorización; no se atribuyen
a la nueva estructura de clases sin repetir el benchmark.

JAR integrable en aplicaciones Java, con una implementación Java de ternas
A/B/C y un puente JNI opcional hacia la biblioteca C existente. El ordenamiento
completo conserva la entrada; frecuencias produce valores únicos y sus cuentas.
No se usa radix ni se reemplaza el ordenamiento completo por `Arrays.sort`.

## Entrega y uso

Archivos generados:

- `dist/tspr3-java-0.1.0.jar`: clases Java 8, sin dependencias externas.
- `dist/tspr3-java-0.1.0-sources.jar`: código fuente Java.
- `dist/java-native/tspr3_jni.dll`: puente opcional Windows x64/MSVC.

Desde una aplicación:

```java
import org.tspr3.Tspr3;

int[] input = {9,8,9,15,12,13,8,8,9};
int[] result = Tspr3.sort(input);
int[] explicit = Tspr3.sort(input, Tspr3.Algorithm.B);
Tspr3.Frequencies counts = Tspr3.frequencies(input);
```

Agregar el JAR al classpath o al empaquetado de dependencias de la aplicación.
Ejemplo verificado con el JAR final:

```powershell
javac -cp dist\tspr3-java-0.1.0.jar -d build\java-client java\examples\UseTspr3.java
java -cp 'build\java-client;dist\tspr3-java-0.1.0.jar' UseTspr3
```

La variante nativa es explícita y nunca se carga al usar solamente `Tspr3`:

```java
import org.tspr3.NativeTspr3;
int[] result = NativeTspr3.sort(input, Tspr3.Algorithm.B);
```

Ejecutar con `-Djava.library.path=dist\java-native` en Windows x64. El DLL debe
corresponder a la arquitectura de la JVM. Para otras plataformas habría que
compilar el puente y el núcleo C para ellas; no se entregan binarios Linux/macOS.
El puente usa `System.loadLibrary`, siguiendo la
[interfaz JNI oficial](https://docs.oracle.com/en/java/javase/21/docs/specs/jni/invocation.html).
Si se cambia el paquete o nombre de `NativeTspr3`, deben cambiar también los
nombres exportados por el puente JNI.

## Compilación y comprobación

```powershell
.\scripts\build-java.ps1
.\scripts\build-java.ps1 -Native
.\scripts\build-java.ps1 -Benchmark
.\scripts\build-java.ps1 -Native -AutoBenchmark
```

Verificado con Eclipse Temurin/HotSpot 1.8.0_502 de 64 bits, `javac -Xlint:all
-Werror`, y MSVC C17 `/O2 /W4 /WX /MD` para JNI. La compilación directa no necesita
Maven ni Gradle. La ruta Maven ahora está verificada con JDK 21 y salida Java 8,
incluidas pruebas, fuentes y Javadoc. El JAR usa bytecode y APIs Java 8. La
refactorización se probó también con Java 21/JNI; los tiempos de esta sección
corresponden al benchmark histórico con Java 8.

El script ejecuta pruebas antes de entregar el JAR. Pasaron 23.352 entradas
en Java puro y con JNI: enumeración exhaustiva de longitudes 0..7, aleatorios,
extremos signed, entrada inmutable, conteos y argumentos inválidos. Un consumidor
separado se compiló y ejecutó contra el JAR, sin depender de las clases internas
del directorio de compilación.

`-Xcheck:jni` produjo advertencias en la inicialización de esta JVM; la advertencia
de referencias locales de `System.initProperties` se reprodujo también con el
consumidor Java puro. Las comprobaciones del puente y todas sus salidas finalizaron
correctamente. No se ejecutó AddressSanitizer sobre el DLL en esta entrega.

## Implementación y diferencias del selector

A ordena ternas y fusiona con comprobaciones de extremos. B compacta las ternas
y suma multiplicidades durante la fusión. C usa el mismo índice de prefijos
comprimido de bloques, conservando ternas cruzadas en grupos padres.

Java usa arreglos primitivos para valores, cuentas, descriptores y nodos. No
crea un objeto por elemento. Las cuentas caben en int porque no pueden superar
la longitud del arreglo. Las cuentas de la salida pertenecen al consumidor y
se entregan en arreglos separados de valores y multiplicidades.

Los pesos de tiempo entrenados para C no se transfieren automáticamente a Java:
la memoria, el JIT y la representación de registros cambian el costo. AUTO Java
usa una política conservadora y documentada hasta disponer de una regresión propia:

1. Para menos de 256 elementos, A.
2. Muestrear hasta 512 ternas distribuidas por la entrada.
3. Si más del 85 % de pares muestreados tienen rangos estrictamente separados, A.
4. Si la repetición en los valores muestreados es al menos 90 %, B; en otro caso A.

Los extremos iguales no se cuentan como separación: esa condición se corrigió
después de observar el caso de 99 % de ceros. C permanece disponible explícitamente.
La muestra solo decide una ruta; cada ruta procesa toda la entrada exactamente.
El perfil usa `Arrays.sort` sobre la muestra acotada para medir repeticiones;
`Arrays.sort` de la entrada completa se usa solo como referencia de pruebas.
AUTO para salida compacta equivale a B. A no produce frecuencias y se rechaza
en esa interfaz. JNI AUTO conserva el selector y los pesos de la biblioteca C.

El Java puro está limitado por el heap de la JVM y puede lanzar `OutOfMemoryError`.
El puente limita el heap auxiliar del núcleo C a 256 MiB, pero añade tres buffers
nativos de transferencia y los arreglos de la JVM, fuera de ese límite. Las
conversiones explícitas entre `jint` e `int32_t` evitan asumir que sus typedefs
son el mismo tipo C. No se mantiene un arreglo bloqueado con JNI durante el sort.

## Mediciones con dos millones de elementos

Se midieron nueve distribuciones con cinco semillas, siete métodos por entrada:
Java A/B/C/AUTO y JNI A/B/C. Las 315 salidas fueron correctas. Se calentaron todas
las rutas antes de medir, el orden rotó entre semillas, y se usó `System.nanoTime`.
Generación, referencia y verificación quedan fuera del tiempo. Las reservas de
salida, trabajo y GC Java, así como las copias JNI, quedan dentro del tiempo.
JVM `-Xms1g -Xmx2g`; sin fijar afinidad ni controlar carga externa.

Medianas de cinco semillas, milisegundos:

| Escenario | Java A | C por JNI A | Java B | C por JNI B |
|---|---:|---:|---:|---:|
| Aleatoria signed | 214,32 | 211,19 | 347,28 | 260,22 |
| Ascendente | 33,26 | 40,34 | 90,95 | 145,44 |
| 8 valores distintos | 109,70 | 108,63 | 74,24 | 76,27 |
| 2048 valores distintos | 170,75 | 151,55 | 188,36 | 152,09 |
| 99 % de ceros | 73,79 | 72,05 | 37,36 | 44,49 |
| Normal amplia | 216,69 | 186,35 | 350,80 | 237,95 |

Java no fue uniformemente más lenta. Estos tiempos comparan implementaciones y
sus interfaces completas, no velocidades intrínsecas de lenguajes: C mantiene
contadores de instrumentación y registros de 16 bytes, mientras Java usa cuentas
int en arreglos separados. JNI añade transferencias. Es un benchmark exploratorio,
no JMH ni un estudio de significancia estadística. No se midió JNI AUTO aquí.

Se repitio el benchmark completo con la condicion corregida de AUTO: las
315 salidas fueron correctas. `java_raw.csv` y `java_summary.csv` contienen
las siete rutas actuales en la misma ejecucion. `java_auto_final.csv` conserva
la comprobacion aislada anterior de 45 salidas. Informe y huella del JAR probado:
[LATEST-TEST.md](LATEST-TEST.md).

Datos: `benchmarks/results/java_raw.csv`, `java_summary.csv`, `java_auto_final.csv`.

## ¿Incorporarlo al núcleo de Java?

La integración actual mediante JAR permite usar el algoritmo sin cambiar el JDK.
HotSpot ya compila y optimiza métodos frecuentes mediante JIT; mover una clase
a la biblioteca estándar no demuestra por sí solo que vaya a ser más rápida.
[Documentación oficial de optimizaciones HotSpot](https://docs.oracle.com/javase/8/docs/technotes/guides/vm/performance-enhancements-7.html).

Modificar OpenJDK sería otro proyecto: habría que demostrar beneficios generales,
definir una API estándar y mantener compatibilidad. La evidencia actual respalda
medir y mejorar esta biblioteca, con JNI como opción cuando resulte conveniente.
No se modificó el JDK ni se propuso un cambio al lenguaje en esta entrega.
