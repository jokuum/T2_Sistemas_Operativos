#include "komikan.h"
#include <stdarg.h>
#include <time.h>

//Estado privado del módulo log.c
static pthread_mutex_t mtx_log = PTHREAD_MUTEX_INITIALIZER;
static FILE *archivo_log = NULL;
static struct timespec inicio;

//miliseconds transcurridos desde log_abrir()
static long ms_desde_inicio(void) {
    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora);
    return (ahora.tv_sec - inicio.tv_sec) * 1000 +
           (ahora.tv_nsec - inicio.tv_nsec) / 1000000;
}

bool log_abrir(const char *ruta) {
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    if (ruta == NULL) return true;              /* solo consola */
    archivo_log = fopen(ruta, "w");
    if (archivo_log == NULL) {
        perror(ruta);
        return false;
    }
    return true;
}

void log_cerrar(void) {
    pthread_mutex_lock(&mtx_log);
    if (archivo_log != NULL) {
        fclose(archivo_log);
        archivo_log = NULL;
    }
    pthread_mutex_unlock(&mtx_log);
}

void log_msg(int mesa, const char *formato, ...) {
    pthread_mutex_lock(&mtx_log);

    long t = ms_desde_inicio();
    FILE *destinos[2] = {stdout, archivo_log};
    for (int i = 0; i < 2; i++) {
        FILE *out = destinos[i];
        if (out == NULL) continue;

        if (mesa >= 0) fprintf(out, "[%6ld ms][Mesa %d] ", t, mesa);
        else           fprintf(out, "[%6ld ms][Servidor] ", t);

        va_list args;
        va_start(args, formato);
        vfprintf(out, formato, args);
        va_end(args);

        fputc('\n', out);
        fflush(out);
    }

    pthread_mutex_unlock(&mtx_log);
}