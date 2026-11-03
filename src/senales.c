#include "komikan.h"
#include <signal.h>

static pthread_t hilo_senales;
static sigset_t  senales;

/* Hilo dedicado: espera señales de forma síncrona con sigwait */
static void *hilo_de_senales(void *arg) {
    Servidor *s = arg;
    int sig;
    while (1) {
        sigwait(&senales, &sig);
        if (sig == SIGINT) {
            log_msg(-1, "SIGINT recibido (Ctrl+C)");
            servidor_apagar(s);
        } else if (sig == SIGUSR1) {   /* aviso interno: el torneo terminó */
            break;
        }
    }
    return NULL;
}

bool senales_iniciar(Servidor *s) {
    sigemptyset(&senales);
    sigaddset(&senales, SIGINT);
    sigaddset(&senales, SIGUSR1);

    /* Bloquear las señales en este hilo: los hilos que se creen después
       heredan la máscara, así que solo el hilo de señales las recibirá */
    if (pthread_sigmask(SIG_BLOCK, &senales, NULL) != 0) {
        return false;
    }
    return pthread_create(&hilo_senales, NULL, hilo_de_senales, s) == 0;
}

void senales_detener(void) {
    pthread_kill(hilo_senales, SIGUSR1);
    pthread_join(hilo_senales, NULL);
}