#ifndef KOMIKAN_H
#define KOMIKAN_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>

// Tablero
#define FILAS            5
#define COLS             5
#define CASILLAS_GRILLA  (FILAS * COLS)
#define CASILLAS_TRONO   0   /* PLACEHOLDER: se fija en la Parte 1 según la figura */
#define NUM_CASILLAS     (CASILLAS_GRILLA + CASILLAS_TRONO)
#define MAX_VECINOS      8

#define NUM_PERROS       12
#define NUM_PIEZAS       (NUM_PERROS + 1)   /* índice 0 = puma, 1..12 = perros */
#define CAPTURAS_VICTORIA 6
#define VACIA            (-1)

typedef enum { PUMA, PERRO } TipoPieza;

typedef enum {
    EN_CURSO,
    GANA_PUMA,
    GANA_PERROS,
    ABANDONO_PUMA,     /* puma falló → ganan perros */
    ABANDONO_PERROS,   /* perros fallaron → gana puma */
    ABORTADA           /* Ctrl+C */
} EstadoPartida;

typedef struct {
    int num_vecinos[NUM_CASILLAS];
    int vecinos[NUM_CASILLAS][MAX_VECINOS];
    int ocupante[NUM_CASILLAS];   /* VACIA o id de pieza (0..12) */
    int capturados;
} Tablero;

// Configuracion
typedef struct {
    int retardo_min_ms;            /* por defecto 50  */
    int retardo_max_ms;            /* por defecto 300 */
    unsigned int semilla;
    int pos_puma;
    int pos_perros[NUM_PERROS];
    double prob_fallo;             /* para probar resiliencia (Parte 5) */
} Config;

// Piezas y Mesas
struct Mesa;

typedef struct {
    int id;                 /* 0..12 */
    TipoPieza tipo;
    int casilla;
    bool viva;              /* false si fue capturada */
    unsigned int rng;       /* estado propio para rand_r */
    pthread_t hilo;
    struct Mesa *mesa;      /* para acceder al tablero compartido */
} Pieza;

typedef struct Mesa {
    int id;
    int jugador_puma;
    int jugador_perros;

    Tablero tablero;
    Pieza piezas[NUM_PIEZAS];

    pthread_mutex_t mtx_tablero;   /* protege tablero y estado */
    pthread_cond_t  cv_fin;        /* el juez espera aquí el fin de partida */
    EstadoPartida estado;
    bool terminar;                 /* señal para que las piezas salgan */

    pthread_t hilo_juez;
    const Config *cfg;
} Mesa;

// Servidor
typedef struct {
    int N;                 /* jugadores totales */
    int M;                 /* mesas simultáneas máximas */
    Config cfg;
    Mesa *mesas;           /* arreglo de M mesas */
    sem_t sem_mesas;       /* cuenta mesas libres */
    pthread_mutex_t mtx_servidor;
    bool apagando;         /* true tras Ctrl+C */
} Servidor;

#endif