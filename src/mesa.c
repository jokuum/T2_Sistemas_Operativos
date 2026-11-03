#include "komikan.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define TABLERO_BUF 1024

/* Imprime el tablero en un buffer para volcarlo de forma atomica en el log */
static void imprimir_tablero_log(const Mesa *m) {
    char buf[TABLERO_BUF];
    int pos = 0;
    const int *o = m->tablero.ocupante;

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

/* Bloquea mtx_tablero y recupera el mutex si el hilo anterior murio reteniendolo */
static int mesa_lock(Mesa *m) {
    int rc = pthread_mutex_lock(&m->mtx_tablero);
    if (rc == EOWNERDEAD) {
        pthread_mutex_consistent(&m->mtx_tablero);
        if (m->estado == EN_CURSO) {
            int fallido = m->dueno_lock;
            if (fallido == 0) {
                m->estado = ABANDONO_PUMA;
            } else {
                m->estado = ABANDONO_PERROS;
            }
            log_msg(m->id, "Pieza %d fallo con lock tomado. Abandono declarado.", fallido);
            pthread_cond_signal(&m->cv_fin);
        }
    }
    return rc;
}

/* Ciclo principal de cada pieza (puma o perro) */
static void *hilo_pieza(void *arg) {
    Pieza *p = (Pieza *)arg;
    Mesa  *m = p->mesa;

    while (1) {
        /* Retardo aleatorio fuera de la seccion critica */
        int ms = aleatorio_rango(&p->rng,
                                 m->cfg->retardo_min_ms,
                                 m->cfg->retardo_max_ms);
        dormir_ms(ms);

        mesa_lock(m);
        m->dueno_lock = p->id;

        /* Salir si la partida concluyo o la pieza fue capturada */
        if (m->terminar || m->estado != EN_CURSO || !p->viva) {
            m->dueno_lock = -1;
            pthread_mutex_unlock(&m->mtx_tablero);
            break;
        }

        /* Simulacion de fallo: la pieza aborta con el lock tomado */
        if (m->cfg->prob_fallo > 0.0 && aleatorio_prob(&p->rng, m->cfg->prob_fallo)) {
            log_msg(m->id, "Pieza %d (%s) sufre fallo interno y aborta",
                    p->id, p->tipo == PUMA ? "PUMA" : "PERRO");
            pthread_exit(NULL);
        }

        /* Recopilar movimientos validos */
        struct Mov {
            int destino;
            int captura_sobre;
        };
        struct Mov opciones[MAX_VECINOS * 2];
        int n_opciones = 0;
        int origen = p->casilla;

        for (int i = 0; i < g_grafo.num_vecinos[origen]; i++) {
            int vec = g_grafo.vecinos[origen][i];

            if (movimiento_valido(&m->tablero, origen, vec)) {
                opciones[n_opciones].destino = vec;
                opciones[n_opciones].captura_sobre = -1;
                n_opciones++;
            }

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

        if (n_opciones > 0) {
            int idx = aleatorio_rango(&p->rng, 0, n_opciones - 1);
            struct Mov elegido = opciones[idx];

            if (elegido.captura_sobre >= 0) {
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
                m->tablero.ocupante[elegido.destino] = p->id;
                m->tablero.ocupante[origen] = VACIA;
                p->casilla = elegido.destino;

                log_msg(m->id, "%s %d: %d -> %d",
                        p->tipo == PUMA ? "PUMA" : "PERRO",
                        p->id, origen, elegido.destino);
            }

            m->movimientos_totales++;
            imprimir_tablero_log(m);

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

        m->dueno_lock = -1;
        pthread_mutex_unlock(&m->mtx_tablero);
    }

    return NULL;
}

/* Hilo juez encargado de coordinar la mesa */
static void *hilo_juez(void *arg) {
    Mesa *m = (Mesa *)arg;

    inicializar_tablero(&m->tablero, m->cfg->pos_puma, m->cfg->pos_perros);

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

    int creadas = 0;
    for (int i = 0; i < NUM_PIEZAS; i++) {
        int rc = pthread_create(&m->piezas[i].hilo, NULL, hilo_pieza, &m->piezas[i]);
        if (rc != 0) {
            log_msg(m->id, "No se pudo crear la pieza %d: %s. Partida abortada",
                    i, strerror(rc));
            mesa_lock(m);
            if (m->estado == EN_CURSO) m->estado = ABORTADA;
            pthread_mutex_unlock(&m->mtx_tablero);
            break;
        }
        creadas++;
    }

    mesa_lock(m);
    while (m->estado == EN_CURSO) {
        int rc = pthread_cond_wait(&m->cv_fin, &m->mtx_tablero);
        if (rc == EOWNERDEAD) {
            pthread_mutex_consistent(&m->mtx_tablero);
            if (m->estado == EN_CURSO) {
                int fallido = m->dueno_lock;
                m->estado = (fallido == 0) ? ABANDONO_PUMA : ABANDONO_PERROS;
                log_msg(m->id, "Pieza %d fallo con lock tomado. Abandono declarado.", fallido);
            }
        }
    }
    m->terminar = true;
    m->dueno_lock = -1;
    pthread_mutex_unlock(&m->mtx_tablero);

    for (int i = 0; i < creadas; i++) {
        pthread_join(m->piezas[i].hilo, NULL);
    }

    const char *resultado;
    switch (m->estado) {
        case GANA_PUMA:        resultado = "GANA PUMA"; break;
        case GANA_PERROS:      resultado = "GANAN PERROS"; break;
        case ABANDONO_PUMA:    resultado = "ABANDONO PUMA (GANAN PERROS)"; break;
        case ABANDONO_PERROS:  resultado = "ABANDONO PERROS (GANA PUMA)"; break;
        case EMPATE:           resultado = "EMPATE"; break;
        case ABORTADA:         resultado = "ABORTADA"; break;
        default:               resultado = "DESCONOCIDO"; break;
    }
    log_msg(m->id, "Partida %d finalizada: %s (movimientos: %d)",
            m->partida, resultado, m->movimientos_totales);

    return NULL;
}

void mesa_inicializar(Mesa *m, int id, const Config *cfg) {
    memset(m, 0, sizeof(Mesa));
    m->id  = id;
    m->cfg = cfg;
    m->estado   = EN_CURSO;
    m->terminar = false;
    m->movimientos_totales = 0;
    m->dueno_lock = -1;

    /* Mutex robusto para permitir recuperacion ante fallos con lock tomado */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&m->mtx_tablero, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&m->cv_fin, NULL);
}

void mesa_preparar(Mesa *m) {
    m->estado              = EN_CURSO;
    m->terminar            = false;
    m->movimientos_totales = 0;
    m->dueno_lock          = -1;
}

EstadoPartida mesa_jugar(Mesa *m, int partida,
                         int jugador_puma, int jugador_perros) {
    m->jugador_puma   = jugador_puma;
    m->jugador_perros = jugador_perros;
    m->partida        = partida;

    int rc = pthread_create(&m->hilo_juez, NULL, hilo_juez, m);
    if (rc != 0) {
        log_msg(m->id, "No se pudo crear el juez: %s. Partida abortada", strerror(rc));
        m->estado = ABORTADA;
        return m->estado;
    }
    pthread_join(m->hilo_juez, NULL);

    return m->estado;
}

void mesa_abortar(Mesa *m) {
    mesa_lock(m);
    if (m->estado == EN_CURSO) {
        m->estado = ABORTADA;
    }
    m->terminar = true;
    pthread_cond_signal(&m->cv_fin);
    pthread_mutex_unlock(&m->mtx_tablero);
}

void mesa_destruir(Mesa *m) {
    pthread_mutex_destroy(&m->mtx_tablero);
    pthread_cond_destroy(&m->cv_fin);
}
