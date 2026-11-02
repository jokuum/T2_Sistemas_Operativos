#include "komikan.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ================================================================
 * CONSTANTE: tamaño del buffer para imprimir el tablero por log.
 * Necesitamos espacio suficiente para el dibujo ASCII completo.
 * ================================================================ */
#define TABLERO_BUF 1024

/* ================================================================
 * HELPER: imprimir_tablero_log
 * 
 * ¿POR QUÉ EXISTE?
 * El imprimir_tablero() original usa printf directo. Si hay varias
 * mesas jugando al mismo tiempo, los printf de una mesa se mezclan
 * con los de otra → sale basura en pantalla.
 * 
 * SOLUCIÓN: Armamos todo el dibujo en un buffer (char buf[]) usando
 * snprintf, y luego lo enviamos completo con log_msg(). Como log_msg
 * tiene su propio mutex (mtx_log), el tablero entero se imprime como
 * un bloque atómico, sin que otra mesa pueda intercalarse.
 * ================================================================ */
static void imprimir_tablero_log(const Mesa *m) {
    char buf[TABLERO_BUF];
    int pos = 0;
    const int *o = m->tablero.ocupante;

    /* Función auxiliar local: convierte un ocupante a su símbolo */
    #define S(i) (o[i] == VACIA ? '.' : (o[i] == 0 ? 'P' : 'D'))

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "\n=== TABLERO KOMIKAN === (Capturas Puma: %d/%d)\n",
        m->tablero.capturados, CAPTURAS_VICTORIA);
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "              %c\n", S(27));
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "            / | \\\n");
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "          %c --+-- %c\n", S(25), S(26));
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "        / |   |   | \\\n");
    for (int r = 0; r < FILAS; r++) {
        int b = r * COLS;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "      %c - %c - %c - %c - %c\n",
            S(b), S(b+1), S(b+2), S(b+3), S(b+4));
        if (r < FILAS - 1)
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                "      |   |   |   |   |\n");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "=============================\n");

    #undef S

    log_msg(m->id, "%s", buf);
}

/* ================================================================
 * FUNCIÓN DEL HILO DE CADA PIEZA (puma o perro)
 * 
 * Cada pieza es un hilo independiente que repite este ciclo:
 *   1. Dormir un tiempo aleatorio (FUERA del mutex)
 *   2. Tomar el mutex
 *   3. Si la partida terminó o fui capturado → salir
 *   4. Buscar un movimiento válido al azar y ejecutarlo
 *   5. Revisar si alguien ganó
 *   6. Soltar el mutex
 * 
 * ¿POR QUÉ DORMIR FUERA DEL MUTEX?
 * Si durmiera CON el mutex tomado, las demás 12 piezas no podrían
 * hacer nada durante 50-300ms. Perderíamos toda la concurrencia.
 * El dormir simula el "tiempo de reacción" de cada pieza. Durante
 * ese tiempo, el tablero DEBE estar disponible para las demás.
 * 
 * ¿POR QUÉ UN SOLO MUTEX POR MESA (y no uno por casilla)?
 * - Con 1 mutex: imposible tener deadlock (necesitas ≥2 locks).
 * - La sección crítica es MUY corta (microsegundos), mientras que
 *   el dormir (50-300ms) es miles de veces más largo y va sin lock.
 * - Con mutex por casilla tendríamos 28 mutexes y riesgo de deadlock.
 * ================================================================ */
static void *hilo_pieza(void *arg) {
    Pieza *p = (Pieza *)arg;
    Mesa  *m = p->mesa;

    while (1) {
        /* ── PASO 1: Dormir FUERA del mutex ── */
        int ms = aleatorio_rango(&p->rng,
                                 m->cfg->retardo_min_ms,
                                 m->cfg->retardo_max_ms);
        dormir_ms(ms);

        /* ── PASO 2: Tomar el mutex ── */
        pthread_mutex_lock(&m->mtx_tablero);

        /* ── PASO 3: ¿Terminó la partida o me capturaron? ── */
        if (m->terminar || !p->viva) {
            pthread_mutex_unlock(&m->mtx_tablero);
            break;
        }

        /* ── PASO 4: Buscar movimientos válidos ── */
        struct Mov {
            int destino;
            int captura_sobre;  /* -1 si es movimiento simple */
        };
        struct Mov opciones[MAX_VECINOS * 2];
        int n_opciones = 0;
        int origen = p->casilla;

        for (int i = 0; i < g_grafo.num_vecinos[origen]; i++) {
            int vec = g_grafo.vecinos[origen][i];

            /* Movimiento simple: vecino vacío */
            if (movimiento_valido(&m->tablero, origen, vec)) {
                opciones[n_opciones].destino = vec;
                opciones[n_opciones].captura_sobre = -1;
                n_opciones++;
            }

            /* Captura (solo el puma): saltar sobre un perro */
            if (p->tipo == PUMA &&
                m->tablero.ocupante[vec] >= 1 &&
                m->tablero.ocupante[vec] <= NUM_PERROS) {
                int dst = destino_salto(origen, vec);
                if (dst != -1 && captura_valida(&m->tablero, origen, vec, dst)) {
                    opciones[n_opciones].destino = dst;
                    opciones[n_opciones].captura_sobre = vec;
                    n_opciones++;
                }
            }
        }

        /* Elegir uno al azar y ejecutarlo */
        if (n_opciones > 0) {
            int idx = aleatorio_rango(&p->rng, 0, n_opciones - 1);
            struct Mov elegido = opciones[idx];

            if (elegido.captura_sobre >= 0) {
                /* ── CAPTURA: puma salta sobre perro ── */
                int id_perro = m->tablero.ocupante[elegido.captura_sobre];
                m->piezas[id_perro].viva = false;
                m->piezas[id_perro].casilla = -1;

                m->tablero.ocupante[elegido.captura_sobre] = VACIA;
                m->tablero.ocupante[elegido.destino] = p->id;
                m->tablero.ocupante[origen] = VACIA;
                p->casilla = elegido.destino;
                m->tablero.capturados++;

                log_msg(m->id, "PUMA captura perro %d: %d -> %d (sobre %d) [capturas: %d/%d]",
                        id_perro, origen, elegido.destino,
                        elegido.captura_sobre,
                        m->tablero.capturados, CAPTURAS_VICTORIA);
            } else {
                /* ── MOVIMIENTO SIMPLE ── */
                m->tablero.ocupante[elegido.destino] = p->id;
                m->tablero.ocupante[origen] = VACIA;
                p->casilla = elegido.destino;

                log_msg(m->id, "%s %d: %d -> %d",
                        p->tipo == PUMA ? "PUMA" : "PERRO",
                        p->id, origen, elegido.destino);
            }

            m->movimientos_totales++;
            imprimir_tablero_log(m);

            /* ── PASO 5: Revisar condiciones de victoria ── */
            if (m->tablero.capturados >= CAPTURAS_VICTORIA) {
                m->estado = GANA_PUMA;
                pthread_cond_signal(&m->cv_fin);
            } else if (!puma_tiene_movimientos(&m->tablero)) {
                m->estado = GANA_PERROS;
                pthread_cond_signal(&m->cv_fin);
            } else if (m->movimientos_totales >= MAX_MOVIMIENTOS) {
                m->estado = EMPATE;
                pthread_cond_signal(&m->cv_fin);
            }
        }

        /* ── PASO 6: Soltar el mutex ── */
        pthread_mutex_unlock(&m->mtx_tablero);
    }

    return NULL;
}

/* ================================================================
 * FUNCIÓN DEL HILO JUEZ
 * ================================================================ */
static void *hilo_juez(void *arg) {
    Mesa *m = (Mesa *)arg;

    /* 1. Inicializar el tablero */
    inicializar_tablero(&m->tablero, m->cfg->pos_puma, m->cfg->pos_perros);

    /* 2. Inicializar las 13 piezas */
    for (int i = 0; i < NUM_PIEZAS; i++) {
        m->piezas[i].id   = i;
        m->piezas[i].tipo = (i == 0) ? PUMA : PERRO;
        m->piezas[i].viva = true;
        m->piezas[i].rng  = semilla_hilo(m->cfg->semilla, m->partida, i);
        m->piezas[i].mesa = m;

        m->piezas[i].casilla = -1;
        for (int c = 0; c < NUM_CASILLAS; c++) {
            if (m->tablero.ocupante[c] == i) {
                m->piezas[i].casilla = c;
                break;
            }
        }
    }

    log_msg(m->id, "Partida %d: Puma(jug %d) vs Perros(jug %d) — inicio",
            m->partida, m->jugador_puma, m->jugador_perros);
    imprimir_tablero_log(m);

    /* 3. Crear los 13 hilos de piezas */
    for (int i = 0; i < NUM_PIEZAS; i++) {
        pthread_create(&m->piezas[i].hilo, NULL, hilo_pieza, &m->piezas[i]);
    }

    /* 4. Esperar el fin de la partida (while contra spurious wakeups) */
    pthread_mutex_lock(&m->mtx_tablero);
    while (m->estado == EN_CURSO) {
        pthread_cond_wait(&m->cv_fin, &m->mtx_tablero);
    }
    m->terminar = true;
    pthread_mutex_unlock(&m->mtx_tablero);

    /* 5. Esperar que todos los hilos terminen */
    for (int i = 0; i < NUM_PIEZAS; i++) {
        pthread_join(m->piezas[i].hilo, NULL);
    }

    /* 6. Anunciar resultado */
    const char *resultado;
    switch (m->estado) {
        case GANA_PUMA:    resultado = "GANA PUMA";    break;
        case GANA_PERROS:  resultado = "GANAN PERROS"; break;
        case EMPATE:       resultado = "EMPATE";       break;
        default:           resultado = "DESCONOCIDO";  break;
    }
    log_msg(m->id, "Partida %d finalizada: %s (movimientos: %d)",
            m->partida, resultado, m->movimientos_totales);

    return NULL;
}

/* ================================================================
 * INTERFAZ PÚBLICA
 * ================================================================ */
void mesa_inicializar(Mesa *m, int id, const Config *cfg) {
    memset(m, 0, sizeof(Mesa));
    m->id  = id;
    m->cfg = cfg;
    m->estado   = EN_CURSO;
    m->terminar = false;
    m->movimientos_totales = 0;
    pthread_mutex_init(&m->mtx_tablero, NULL);
    pthread_cond_init(&m->cv_fin, NULL);
}

EstadoPartida mesa_jugar(Mesa *m, int partida,
                         int jugador_puma, int jugador_perros) {
    m->jugador_puma   = jugador_puma;
    m->jugador_perros = jugador_perros;
    m->partida        = partida;
    m->estado         = EN_CURSO;
    m->terminar       = false;
    m->movimientos_totales = 0;

    pthread_create(&m->hilo_juez, NULL, hilo_juez, m);
    pthread_join(m->hilo_juez, NULL);

    return m->estado;
}

void mesa_destruir(Mesa *m) {
    pthread_mutex_destroy(&m->mtx_tablero);
    pthread_cond_destroy(&m->cv_fin);
}
