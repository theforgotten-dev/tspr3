# Ordenacion generica interna

`src/c/internal/tspr3_generic_tri.h` declara el motor experimental de registros.
Se compila con la biblioteca C, pero su header no se instala ni se expone aun
mediante las fachadas publicas C++, Java o JNI.

La reparacion conserva todos los registros: igualdad de clave no implica
igualdad de contenido. Cada llamada ordena referencias a la entrada y copia
los registros completos a la salida solamente despues de terminar correctamente.
El desempate por posicion original garantiza estabilidad. Los callbacks reciben
las direcciones originales; no se les pasan copias desalineadas de estructuras.

BASE conserva las ternas y las fusiones por pares. COUNTS conserva el motor de
registros internos, pero no compacta objetos con claves iguales. BIN conserva
el arbol de prefijos sobre claves de 64 bits; la clave numerica debe respetar
el orden del comparador. Se admiten colisiones, incluso una clave constante.
Una clave numerica incompatible con el comparador incumple el contrato.

La reserva de referencias y arenas se comprueba antes de multiplicar tamanos.
No se reserva memoria por cada terna. Los fallos de asignacion devuelven
`TSPR3_NO_MEMORY`, liberan los buffers y conservan la salida intacta.
`distinct_values` permanece en cero: no se cuentan claves distintas en esta API.
La cota de memoria incluye referencias y arenas; excluye la entrada y salida.
Entrada y salida deben ser buffers disjuntos.

Validacion local del 3 de noviembre de 2026:

- CMake Release/MSVC: cinco pruebas, incluidos los contratos existentes.
- Comparacion contra una referencia independiente por clave y posicion original,
  longitudes 0..151, payloads grandes y entrada inmutable.
- Registros de tres bytes y claves de prefijo con colisiones.
- Fallas de asignacion inyectadas, comprobacion de liberaciones y salida intacta.
- Prueba generica con AddressSanitizer y compilacion `/W4 /WX`.
- Caso de regresion: tres registros de igual clave con IDs 10,20,30 mantienen
  esos IDs en BASE, COUNTS y BIN.

No se han medido mejoras de rendimiento. COUNTS generico no conserva el ahorro
de compactar enteros repetidos, porque debe preservar la identidad de los objetos.
