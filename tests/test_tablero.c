#include "komikan.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void test_vecinos(void) {
    printf("[TEST] Verificando cantidad de vecinos...\n");

    // Centro de la grilla (casilla 12: fila 2, col 2) -> 4 ortogonales + 4 diagonales = 8
    assert(g_grafo.num_vecinos[12] == 8);

    // Esquina inferior derecha (casilla 24: fila 4, col 4) -> 2 ortogonales + 1 diagonal = 3
    assert(g_grafo.num_vecinos[24] == 3);

    // Borde inferior sin diagonales (casilla 21: fila 4, col 1) -> 3 ortogonales = 3
    assert(g_grafo.num_vecinos[21] == 3);

    // Cúspide del trono T (casilla 28) -> conecta con 25, 26, 27 = 3
    assert(g_grafo.num_vecinos[28] == 3);

    printf("  -> Vecinos verificados correctamente.\n");
}

static void test_saltos(void) {
    printf("[TEST] Verificando saltos en linea recta...\n");

    // Salto válido en la grilla (desde el centro hacia arriba: 12 sobre 7 -> 2)
    assert(destino_salto(12, 7) == 2);
    assert(destino_salto(2, 7) == 12);

    // Salto diagonal válido en la grilla (12 sobre 6 -> 0)
    assert(destino_salto(12, 6) == 0);

    // Salto válido dentro del trono (25 sobre 26 -> 27)
    assert(destino_salto(25, 26) == 27);

    // Salto válido trono a grilla (28 sobre 26 -> 2)
    assert(destino_salto(28, 26) == 2);

    // Salto inválido (casilla 21 salta sobre 20 -> saldría de la grilla)
    assert(destino_salto(21, 20) == -1);

    // Salto entre casillas no adyacentes
    assert(destino_salto(0, 24) == -1);

    printf("  -> Saltos verificados correctamente.\n");
}

static void test_puma_acorralado(void) {
    printf("[TEST] Verificando deteccion de Puma acorralado...\n");

    Tablero t;
    for (int i = 0; i < NUM_CASILLAS; i++) t.ocupante[i] = VACIA;
    t.capturados = 0;

    // Colocamos al Puma en la esquina 24 (fila 4, col 4)
    // Sus vecinos son: 19 (arriba), 23 (izq), 17 (diagonal arriba-izq)
    t.ocupante[24] = 0; // Puma

    // Colocamos perros rodeándolo en todos sus vecinos
    t.ocupante[19] = 1; // Perro 1
    t.ocupante[23] = 2; // Perro 2
    t.ocupante[17] = 3; // Perro 3

    // Bloqueamos también los destinos de sus saltos para que no pueda capturar:
    // Salto sobre 19 aterrizaría en 14: bloqueado con perro 4
    t.ocupante[14] = 4;
    // Salto sobre 23 aterrizaría en 22: bloqueado con perro 5
    t.ocupante[22] = 5;
    // Salto sobre 17 aterrizaría en 10 (diagonal): bloqueado con perro 6
    t.ocupante[10] = 6;

    // El puma no tiene a dónde moverse ni a quién capturar
    assert(!puma_tiene_movimientos(&t));

    // Si liberamos la casilla 14, ahora puede capturar al perro 19
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

