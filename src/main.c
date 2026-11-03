#include "komikan.h"
#include <stdlib.h>

/* Convierte un argumento a entero positivo. Devuelve -1 si no es válido. */
static int leer_positivo(const char *txt) {
    int valor;
    char sobra;
    if (sscanf(txt, "%d%c", &valor, &sobra) != 1 || valor <= 0) {
        return -1;
    }
    return valor;
}

int main(int argc, char *argv[]) {
    /* 1. Cantidad de argumentos: ./torneo N M [config.txt] */
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s N M [config.txt]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 2. N y M deben ser enteros positivos */
    int N = leer_positivo(argv[1]);
    int M = leer_positivo(argv[2]);
    if (N < 0 || M < 0) {
        fprintf(stderr, "Error: N y M deben ser enteros positivos\n");
        return EXIT_FAILURE;
    }
    if (N % 2 != 0) {
        fprintf(stderr, "Aviso: N=%d es impar; el último jugador quedará sin pareja\n", N);
    }

    /* 3. Configuración: del archivo si se entregó, si no por defecto */
    Config cfg;
    const char *ruta = (argc == 4) ? argv[3] : NULL;
    if (!config_cargar(&cfg, ruta)) {
        return EXIT_FAILURE;
    }

    /* 4. Grafo del tablero: una sola vez, antes de crear hilos */
    inicializar_grafo();

    /* 5. Abrir el log (consola + archivo) */
    log_abrir("torneo.log");

    log_msg(-1, "Torneo Komikan: N=%d jugadores, M=%d mesas", N, M);

    /* 6. Servidor / Matchmaker */
    Servidor srv;
    if (!servidor_inicializar(&srv, N, M, &cfg)) {
        log_cerrar();
        return EXIT_FAILURE;
    }

    /* Señales: bloquear SIGINT y crear el hilo de señales ANTES de crear
       los hilos del torneo, para que todos hereden la máscara */
    if (!senales_iniciar(&srv)) {
        servidor_destruir(&srv);
        log_cerrar();
        return EXIT_FAILURE;
    }

    servidor_ejecutar(&srv);

    /* Detener el hilo de señales ANTES de destruir el servidor,
       para que nadie llame a servidor_apagar sobre recursos destruidos */
    senales_detener();
    servidor_destruir(&srv);

    /* 7. Cerrar el log */
    log_cerrar();

    return EXIT_SUCCESS;
}