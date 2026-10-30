#include "komikan.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void test_vecinos(void) {
    printf("[TEST] Verificando cantidad de vecinos...\n");
    assert(g_grafo.num_vecinos[12] == 4);  // centro: arriba, abajo, izq, der
    assert(g_grafo.num_vecinos[24] == 2);  // esquina inferior derecha
    assert(g_grafo.num_vecinos[0]  == 3);  // 1, 5 y 25 (trono)
    assert(g_grafo.num_vecinos[1]  == 4);  // 0, 2, 6 y 25 (trono)
    assert(g_grafo.num_vecinos[27] == 3);  // T: 25, 26 y 2
    assert(son_adyacentes(25, 26));        // horizontal del trono
    assert(!son_adyacentes(12, 18));       // ya no hay diagonales
    printf("  -> Vecinos verificados correctamente.\n");
}

static void test_saltos(void) {
    printf("[TEST] Verificando saltos en linea recta...\n");
    assert(destino_salto(12, 7) == 2);     // grilla, hacia arriba
    assert(destino_salto(2, 7) == 12);     // y de vuelta
    assert(destino_salto(27, 25) == 0);    // lado izquierdo del trono
    assert(destino_salto(6, 1) == 25);     // vertical que entra al trono
    assert(destino_salto(27, 2) == 7);     // vertical central
    assert(destino_salto(25, 26) == -1);   // sin casilla intermedia
    assert(destino_salto(12, 6) == -1);    // diagonal: ya no existe
    assert(destino_salto(21, 20) == -1);   // se saldría de la grilla
    printf("  -> Saltos verificados correctamente.\n");
}

static void test_puma_acorralado(void) {
    printf("[TEST] Verificando deteccion de Puma acorralado...\n");
    Tablero t;
    for (int i = 0; i < NUM_CASILLAS; i++) t.ocupante[i] = VACIA;
    t.capturados = 0;

    // Puma en la esquina 24: sus únicos vecinos son 19 y 23
    t.ocupante[24] = 0;
    t.ocupante[19] = 1;
    t.ocupante[23] = 2;
    // Bloqueamos los destinos de salto: sobre 19 -> 14, sobre 23 -> 22
    t.ocupante[14] = 3;
    t.ocupante[22] = 4;
    assert(!puma_tiene_movimientos(&t));

    // Liberamos 14: ahora puede capturar al perro de 19
    t.ocupante[14] = VACIA;
    assert(puma_tiene_movimientos(&t));
    assert(captura_valida(&t, 24, 19, 14));
    printf("  -> Deteccion de acorralamiento verificada con exito.\n");
}

int main(void) {
    printf("=========================================\n");
    printf("   TESTS UNITARIOS DE TABLERO Y REGLAS   \n");
    printf("=========================================\n\n");

    inicializar_grafo();

    test_vecinos();
    test_saltos();
    test_puma_acorralado();

    // Probar tablero inicial e imprimirlo
    Tablero inicial;
    inicializar_tablero(&inicial);
    assert(puma_tiene_movimientos(&inicial));

    printf("\nVisualizacion del tablero inicial:\n");
    imprimir_tablero(&inicial);

    printf("TODAS LAS PRUEBAS PASARON EXITOSAMENTE!\n");
    return EXIT_SUCCESS;
}

