# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos. Servidor de torneo del juego Komikan (1 Puma vs 12 Perros)
en C con hilos POSIX: N jugadores se emparejan y juegan en hasta M mesas simultáneas, sin
turnos, con cada pieza como un hilo independiente.

## Requisitos

- **Linux** con `gcc` y pthreads.
- No funciona en macOS: usa mutex robustos (`PTHREAD_MUTEX_ROBUST`) y `sem_init`, que macOS no soporta.

## Compilar

```bash
make
```

Sin Makefile (mismos flags que pide el enunciado):

```bash
gcc -Wall -Wextra -std=c17 -pthread -Iinclude src/*.c -o torneo
```

## Ejecutar

```bash
./torneo N M [config.txt]
```

- `N`: cantidad de jugadores (entero positivo). Si es impar, el último se retira sin rival.
- `M`: máximo de mesas simultáneas (entero positivo).
- `config.txt`: opcional. Sin él se usan los valores por defecto.

Ejemplos:

```bash
./torneo 8 2 configs/default.txt     # torneo normal
./torneo 200 3 configs/rapido.txt    # carga alta, máxima competencia
./torneo 20 3 configs/fallos.txt     # con fallos simulados (abandonos)
```

La salida (eventos y tablero en cada movimiento) se muestra en consola y se guarda en
`torneo.log`. **Ctrl+C** apaga el servidor de forma ordenada: rechaza nuevos jugadores,
aborta las partidas en curso y libera todos los recursos.

## Archivo de configuración

Formato `clave = valor`, una por línea. Las líneas que empiezan con `#` son comentarios.
Todas las claves son opcionales: las que faltan toman su valor por defecto.

| Clave         | Descripción                                   | Por defecto |
|---------------|-----------------------------------------------|-------------|
| `retardo_min` | Retardo mínimo entre movimientos de una pieza (ms) | 50 |
| `retardo_max` | Retardo máximo (ms), como máximo 10000         | 300 |
| `semilla`     | Semilla aleatoria (reproducibilidad)          | hora actual |
| `puma`        | Casilla inicial del puma                      | 27 |
| `perros`      | 12 casillas iniciales, separadas por coma     | `10,14,15,...,24` |
| `prob_fallo`  | Probabilidad de fallo por ciclo de cada pieza (0 a 1) | 0.0 |

La configuración usada, incluida la semilla, se escribe al inicio del log. Una config
inválida (retardos invertidos, posiciones repetidas o fuera del tablero, etc.) se rechaza
con un mensaje antes de iniciar.

### Numeración de casillas

```
              27
            / | \
          25 -+- 26
        / |   |   | \
       0 - 1 - 2 - 3 - 4
       |   |   |   |   |
       5 - 6 - 7 - 8 - 9
       |   |   |   |   |
      10 -11 -12 -13 -14
       |   |   |   |   |
      15 -16 -17 -18 -19
       |   |   |   |   |
      20 -21 -22 -23 -24
```

Grilla 5×5 sin diagonales (`id = fila*5 + col`) más el trono (25, 26 y 27 = cúspide T).

### Configuraciones incluidas (`configs/`)

| Archivo         | Uso |
|-----------------|-----|
| `default.txt`   | Valores estándar del enunciado (50–300 ms, semilla 42). |
| `rapido.txt`    | Retardos de 0–5 ms para forzar la competencia por casillas. |
| `fallos.txt`    | `prob_fallo = 0.002`: piezas que fallan con el lock tomado → abandono. |
| `invertido.txt` | Disposición inicial distinta (puma al centro, perros arriba). |
| `malo.txt`      | Inválida a propósito: el programa debe rechazarla. |

## Pruebas

```bash
make test        # tests de tablero, log thread-safe y aleatoriedad reproducible
```

Verificación de memoria, descriptores y condiciones de carrera:

```bash
valgrind --leak-check=full --track-fds=yes ./torneo 8 2 configs/rapido.txt
valgrind --tool=helgrind ./torneo 6 2 configs/rapido.txt
```

Con `fallos.txt`, helgrind reporta errores esperables: no modela la recuperación de mutex
robustos tras la muerte de un hilo.

## Estructura

| Archivo           | Contenido |
|-------------------|-----------|
| `include/komikan.h` | Constantes, structs y prototipos compartidos. |
| `src/main.c`      | Argumentos, inicialización y orden de arranque/apagado. |
| `src/config.c`    | Lectura y validación de la configuración. |
| `src/tablero.c`   | Grafo del tablero y reglas (movimientos, capturas, acorralamiento). |
| `src/mesa.c`      | Juez y piezas de una mesa: sincronización del tablero, fallos y abandono. |
| `src/servidor.c`  | Matchmaker: llegada, emparejamiento y semáforo de M mesas. |
| `src/senales.c`   | Hilo dedicado a señales (Ctrl+C) con `sigwait`. |
| `src/log.c`       | Log thread-safe a consola y archivo. |
| `src/aleatorio.c` | Aleatoriedad reproducible por hilo (`rand_r`) y retardos. |
| `tests/`          | Pruebas unitarias. |

## Notas

- Una partida que llega a 1000 movimientos sin ganador termina en **empate**, para evitar partidas infinitas.
- La semilla fija las decisiones de cada pieza, pero el orden de ejecución de los hilos lo decide el sistema operativo, por lo que dos ejecuciones con la misma semilla pueden diferir.

## Autores

- Joaquín Utreras
- Jeremias Quezada