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

    /* 6. Probar UNA partida en UNA mesa (Parte 3).
     *    En la Parte 4, esto lo reemplaza el Matchmaker. */
    Mesa mesa;
    mesa_inicializar(&mesa, 0, &cfg);
    EstadoPartida resultado = mesa_jugar(&mesa, 0, 1, 2);

    const char *txt_resultado;
    switch (resultado) {
        case GANA_PUMA:    txt_resultado = "GANA PUMA";    break;
        case GANA_PERROS:  txt_resultado = "GANAN PERROS"; break;
        case EMPATE:       txt_resultado = "EMPATE";       break;
        default:           txt_resultado = "DESCONOCIDO";  break;
    }
    log_msg(-1, "Resultado final: %s", txt_resultado);

    mesa_destruir(&mesa);

    /* 7. Cerrar el log */
    log_cerrar();

    return EXIT_SUCCESS;
}
