#include "komikan.h"
#include <stdlib.h>

static void *hilo(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) log_msg(id, "mensaje %d del hilo %d", i, id);
    return NULL;
}

int main(void) {
    if (!log_abrir("test.log")) return 1;
    pthread_t h[4];
    int ids[4];
    for (int i = 0; i < 4; i++) {
        ids[i] = i;
        pthread_create(&h[i], NULL, hilo, &ids[i]);
    }
    for (int i = 0; i < 4; i++) pthread_join(h[i], NULL);
    log_msg(-1, "fin");
    log_cerrar();
    return 0;
}