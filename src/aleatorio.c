#include "komikan.h"
#include <stdlib.h>
#include <time.h>

unsigned int semilla_hilo(unsigned int global, int partida, int pieza) {
    return global + (unsigned int) (partida * NUM_PIEZAS + pieza) * 2654435761u;
}

int aleatorio_rango(unsigned int *estado, int min, int max) {
    return min + rand_r(estado) % (max - min + 1);
}

bool aleatorio_prob(unsigned int *estado, double p) {
    return (double) rand_r(estado) / RAND_MAX < p;
}

void dormir_ms(int ms) {
    struct timespec t = { ms / 1000, (long) (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}