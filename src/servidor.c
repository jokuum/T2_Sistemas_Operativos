#include "komikan.h"
#include <stdlib.h>
#include <string.h>

// Creacion y destruccion 

bool servidor_inicializar(Servidor *s, int N, int M, const Config *cfg) {
    s->N = N;
    s->M = M;
    s->cfg = cfg;
    s->esperando = -1;
    s->llegados  = 0;
    s->partidas  = 0;
    s->apagando  = false;

    s->mesas        = malloc(M * sizeof(Mesa));
    s->mesa_ocupada = calloc(M, sizeof(bool));
    if (s->mesas == NULL || s->mesa_ocupada == NULL) {
        perror("malloc");
        free(s->mesas);
        free(s->mesa_ocupada);
        return false;
    }
    for (int i = 0; i < M; i++) {
        mesa_inicializar(&s->mesas[i], i, cfg);
    }

    /* Semáforo sin nombre, compartido entre hilos (pshared = 0), parte en M */
    if (sem_init(&s->sem_mesas, 0, (unsigned) M) != 0) {
        perror("sem_init");
        for (int i = 0; i < M; i++) mesa_destruir(&s->mesas[i]);
        free(s->mesas);
        free(s->mesa_ocupada);
        return false;
    }

    pthread_mutex_init(&s->mtx_servidor, NULL);
    pthread_cond_init(&s->cv_pareja, NULL);
    return true;
}

void servidor_destruir(Servidor *s) {
    sem_destroy(&s->sem_mesas);
    pthread_mutex_destroy(&s->mtx_servidor);
    pthread_cond_destroy(&s->cv_pareja);
    for (int i = 0; i < s->M; i++) {
        mesa_destruir(&s->mesas[i]);
    }
    free(s->mesas);
    free(s->mesa_ocupada);
}

//Jugar una partida en una mesa libre

static void jugar_partida(Servidor *s, int jugador_puma, int jugador_perros, int partida) {
    /* 1. Esperar (pasivamente) a que haya una mesa libre */
    log_msg(-1, "Par (%d, %d) espera mesa", jugador_puma, jugador_perros);
    sem_wait(&s->sem_mesas);

    /* 2. Reservar una mesa libre (salvo que el servidor se esté apagando) */
    pthread_mutex_lock(&s->mtx_servidor);
    if (s->apagando) {
        pthread_mutex_unlock(&s->mtx_servidor);
        log_msg(-1, "Par (%d, %d) cancelado: servidor apagándose",
                jugador_puma, jugador_perros);
        sem_post(&s->sem_mesas);   /* despierta al siguiente par en espera */
        return;
    }
    int id_mesa = -1;
    for (int i = 0; i < s->M; i++) {
        if (!s->mesa_ocupada[i]) {
            s->mesa_ocupada[i] = true;
            id_mesa = i;
            break;
        }
    }
    mesa_preparar(&s->mesas[id_mesa]);
    pthread_mutex_unlock(&s->mtx_servidor);

    log_msg(-1, "Partida %d: jugadores %d (puma) y %d (perros) -> Mesa %d",
            partida, jugador_puma, jugador_perros, id_mesa);

    /* 3. Jugar (bloquea hasta que termina la partida) */
    mesa_jugar(&s->mesas[id_mesa], partida, jugador_puma, jugador_perros);

    /* 4. Liberar la mesa y avisar al semáforo */
    pthread_mutex_lock(&s->mtx_servidor);
    s->mesa_ocupada[id_mesa] = false;
    pthread_mutex_unlock(&s->mtx_servidor);
    log_msg(-1, "Mesa %d liberada", id_mesa);
    sem_post(&s->sem_mesas);
}

//Hilo de cada jugador 

static void *hilo_cliente(void *arg) {
    Cliente  *c = arg;
    Servidor *s = c->srv;

    /* Simular que los jugadores llegan en momentos distintos */
    unsigned int rng = s->cfg->semilla + (unsigned int) (c->id + 1) * 40503u;
    dormir_ms(aleatorio_rango(&rng, 0, 200));

    pthread_mutex_lock(&s->mtx_servidor);
    if (s->apagando) {
        pthread_mutex_unlock(&s->mtx_servidor);
        log_msg(-1, "Jugador %d rechazado: servidor apagándose", c->id);
        return NULL;
    }
    s->llegados++;
    log_msg(-1, "Llega jugador %d (%d/%d)", c->id, s->llegados, s->N);

    if (s->esperando == -1) {
        if (s->llegados == s->N) {
            /* Último en llegar y nadie esperando: N impar */
            log_msg(-1, "Jugador %d no tiene rival (N impar), se retira", c->id);
            pthread_mutex_unlock(&s->mtx_servidor);
            return NULL;
        }
        /* Nadie esperando: quedo en espera hasta que llegue un rival */
        s->esperando = c->id;
        while (s->esperando == c->id && !s->apagando) {
            pthread_cond_wait(&s->cv_pareja, &s->mtx_servidor);
        }
        if (s->esperando == c->id) {          /* despertó por Ctrl+C, sin rival */
            s->esperando = -1;
            log_msg(-1, "Jugador %d deja de esperar: servidor apagándose", c->id);
        }
        pthread_mutex_unlock(&s->mtx_servidor);
        return NULL;   /* mi rival se encarga de la partida */
    }

    /* Hay alguien esperando: formamos pareja */
    int rival   = s->esperando;
    int partida = s->partidas++;
    s->esperando = -1;
    pthread_cond_signal(&s->cv_pareja);
    pthread_mutex_unlock(&s->mtx_servidor);

    jugar_partida(s, rival, c->id, partida);
    return NULL;
}

// Ejecutar el torneo

void servidor_ejecutar(Servidor *s) {
    Cliente *clientes = malloc(s->N * sizeof(Cliente));
    if (clientes == NULL) {
        perror("malloc");
        return;
    }
        int creados = 0;
    for (int i = 0; i < s->N; i++) {
        clientes[i].id  = i;
        clientes[i].srv = s;
        int rc = pthread_create(&clientes[i].hilo, NULL, hilo_cliente, &clientes[i]);
        if (rc != 0) {
            log_msg(-1, "No se pudo crear el hilo del jugador %d: %s",
                    i, strerror(rc));
            servidor_apagar(s);   /* despierta a quien quedó esperando rival */
            break;
        }
        creados++;
    }
    for (int i = 0; i < creados; i++) {
        pthread_join(clientes[i].hilo, NULL);
    }
    free(clientes);
    log_msg(-1, "Torneo %s: %d parejas formadas",
            s->apagando ? "interrumpido" : "finalizado", s->partidas);
}

// Apagado ordenado (Ctrl+C)

void servidor_apagar(Servidor *s) {
    pthread_mutex_lock(&s->mtx_servidor);
    if (s->apagando) {                       /* ya se estaba apagando */
        pthread_mutex_unlock(&s->mtx_servidor);
        return;
    }
    s->apagando = true;
    log_msg(-1, "Apagando servidor...");
    /* 1. Despertar al jugador que espera rival */
    pthread_cond_broadcast(&s->cv_pareja);

    /* 2. Pedir a cada partida activa que termine */
    for (int i = 0; i < s->M; i++) {
        if (s->mesa_ocupada[i]) {
            mesa_abortar(&s->mesas[i]);
        }
    }
    pthread_mutex_unlock(&s->mtx_servidor);
}