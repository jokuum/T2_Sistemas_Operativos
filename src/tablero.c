#include "komikan.h"
#include <string.h>
#include <stdio.h>

/* Instancia global única del grafo (inmutable en ejecución) */
GrafoTablero g_grafo;

// Helper estático: añade arista no dirigida entre u y v si no existe
static void agregar_arista(int u, int v) {
    if (u < 0 || u >= NUM_CASILLAS || v < 0 || v >= NUM_CASILLAS || u == v) {
        return;
    }
    // Verificar si ya son vecinos
    for (int i = 0; i < g_grafo.num_vecinos[u]; i++) {
        if (g_grafo.vecinos[u][i] == v) return;
    }
    if (g_grafo.num_vecinos[u] < MAX_VECINOS) {
        g_grafo.vecinos[u][g_grafo.num_vecinos[u]++] = v;
    }
    if (g_grafo.num_vecinos[v] < MAX_VECINOS) {
        g_grafo.vecinos[v][g_grafo.num_vecinos[v]++] = u;
    }
}

// Helper estático: define un salto en línea recta bidireccional
// (de origen sobre sobre aterriza en destino, y viceversa)
static void definir_salto(int origen, int sobre, int destino) {
    if (origen < 0 || origen >= NUM_CASILLAS ||
        sobre < 0 || sobre >= NUM_CASILLAS ||
        destino < 0 || destino >= NUM_CASILLAS) {
        return;
    }
    g_grafo.salto[origen][sobre] = destino;
    g_grafo.salto[destino][sobre] = origen;
}

void inicializar_grafo(void) {
    memset(&g_grafo, 0, sizeof(g_grafo));
    for (int i = 0; i < NUM_CASILLAS; i++) {
        for (int j = 0; j < NUM_CASILLAS; j++) {
            g_grafo.salto[i][j] = -1;
        }
    }

    // 1. Conexiones en la grilla 5x5 (casillas 0 a 24)
    for (int r = 0; r < FILAS; r++) {
        for (int c = 0; c < COLS; c++) {
            int u = r * COLS + c;

            // Ortogonales
            if (c + 1 < COLS) agregar_arista(u, r * COLS + (c + 1));
            if (r + 1 < FILAS) agregar_arista(u, (r + 1) * COLS + c);

            // Diagonales (solo si fila + col es par)
            if ((r + c) % 2 == 0) {
                if (r + 1 < FILAS && c + 1 < COLS) {
                    agregar_arista(u, (r + 1) * COLS + (c + 1));
                }
                if (r + 1 < FILAS && c - 1 >= 0) {
                    agregar_arista(u, (r + 1) * COLS + (c - 1));
                }
            }
        }
    }

    // 2. Saltos en línea recta dentro de la grilla 5x5
    int dir_orto[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    int dir_diag[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int r = 0; r < FILAS; r++) {
        for (int c = 0; c < COLS; c++) {
            int u = r * COLS + c;

            // Saltos ortogonales (distancia 2)
            for (int d = 0; d < 4; d++) {
                int r_mid = r + dir_orto[d][0];
                int c_mid = c + dir_orto[d][1];
                int r_dst = r + 2 * dir_orto[d][0];
                int c_dst = c + 2 * dir_orto[d][1];

                if (r_dst >= 0 && r_dst < FILAS && c_dst >= 0 && c_dst < COLS) {
                    int v_mid = r_mid * COLS + c_mid;
                    int v_dst = r_dst * COLS + c_dst;
                    g_grafo.salto[u][v_mid] = v_dst;
                }
            }

            // Saltos diagonales (solo si r + c es par)
            if ((r + c) % 2 == 0) {
                for (int d = 0; d < 4; d++) {
                    int r_mid = r + dir_diag[d][0];
                    int c_mid = c + dir_diag[d][1];
                    int r_dst = r + 2 * dir_diag[d][0];
                    int c_dst = c + 2 * dir_diag[d][1];

                    if (r_dst >= 0 && r_dst < FILAS && c_dst >= 0 && c_dst < COLS) {
                        int v_mid = r_mid * COLS + c_mid;
                        int v_dst = r_dst * COLS + c_dst;
                        g_grafo.salto[u][v_mid] = v_dst;
                    }
                }
            }
        }
    }

    // 3. Conexiones del Trono (25: izq, 26: centro, 27: der, 28: T)
    // Dentro del triángulo
    agregar_arista(28, 25);
    agregar_arista(28, 26);
    agregar_arista(28, 27);
    agregar_arista(25, 26);
    agregar_arista(26, 27);

    // Conexión del triángulo con la fila 0 de la grilla (casillas 0, 2 y 4)
    agregar_arista(25, 0);
    agregar_arista(25, 2);
    agregar_arista(26, 0);
    agregar_arista(26, 2);
    agregar_arista(26, 4);
    agregar_arista(27, 2);
    agregar_arista(27, 4);

    // 4. Saltos en línea recta del Trono y frontera con la grilla
    // Eje vertical central: 28 <-> 26 <-> 2 <-> 7
    definir_salto(28, 26, 2);
    definir_salto(26, 2, 7);

    // Diagonal izquierda: 28 <-> 25 <-> 0 <-> 6
    definir_salto(28, 25, 0);
    definir_salto(25, 0, 6);

    // Diagonal derecha: 28 <-> 27 <-> 4 <-> 8
    definir_salto(28, 27, 4);
    definir_salto(27, 4, 8);

    // Horizontal intermedia del trono: 25 <-> 26 <-> 27
    definir_salto(25, 26, 27);

    // Diagonales cruzadas que pasan por casilla 2:
    // 25 <-> 2 <-> 8 (fila 1, col 3)
    definir_salto(25, 2, 8);
    // 27 <-> 2 <-> 6 (fila 1, col 1)
    definir_salto(27, 2, 6);
}

bool son_adyacentes(int a, int b) {
    if (a < 0 || a >= NUM_CASILLAS || b < 0 || b >= NUM_CASILLAS) {
        return false;
    }
    for (int i = 0; i < g_grafo.num_vecinos[a]; i++) {
        if (g_grafo.vecinos[a][i] == b) {
            return true;
        }
    }
    return false;
}

int destino_salto(int origen, int sobre) {
    if (origen < 0 || origen >= NUM_CASILLAS || sobre < 0 || sobre >= NUM_CASILLAS) {
        return -1;
    }
    return g_grafo.salto[origen][sobre];
}

void inicializar_tablero(Tablero *t) {
    if (!t) return;
    for (int i = 0; i < NUM_CASILLAS; i++) {
        t->ocupante[i] = VACIA;
    }
    t->capturados = 0;

    // Disposición estándar inicial de Komikan:
    // Puma (id 0) en el centro de la grilla (fila 2, col 2 -> id 12)
    t->ocupante[12] = 0;

    // 12 Perros (ids 1..12) en las filas inferiores:
    // Fila 4 (20..24): 5 perros
    t->ocupante[20] = 1;
    t->ocupante[21] = 2;
    t->ocupante[22] = 3;
    t->ocupante[23] = 4;
    t->ocupante[24] = 5;

    // Fila 3 (15..19): 5 perros
    t->ocupante[15] = 6;
    t->ocupante[16] = 7;
    t->ocupante[17] = 8;
    t->ocupante[18] = 9;
    t->ocupante[19] = 10;

    // Fila 2 extremos (10 y 14): 2 perros
    t->ocupante[10] = 11;
    t->ocupante[14] = 12;
}

bool movimiento_valido(const Tablero *t, int origen, int destino) {
    if (!t) return false;
    if (origen < 0 || origen >= NUM_CASILLAS || destino < 0 || destino >= NUM_CASILLAS) {
        return false;
    }
    if (t->ocupante[origen] == VACIA) {
        return false; // No hay pieza que mover
    }
    if (t->ocupante[destino] != VACIA) {
        return false; // Casilla destino ocupada
    }
    return son_adyacentes(origen, destino);
}

bool captura_valida(const Tablero *t, int origen, int sobre, int destino) {
    if (!t) return false;
    if (origen < 0 || origen >= NUM_CASILLAS ||
        sobre < 0 || sobre >= NUM_CASILLAS ||
        destino < 0 || destino >= NUM_CASILLAS) {
        return false;
    }
    // Solo el puma (id 0) captura
    if (t->ocupante[origen] != 0) {
        return false;
    }
    // Debe haber un perro en la casilla intermedia (ids 1..12)
    if (t->ocupante[sobre] < 1 || t->ocupante[sobre] > NUM_PERROS) {
        return false;
    }
    // Destino debe estar vacío
    if (t->ocupante[destino] != VACIA) {
        return false;
    }
    // El destino debe coincidir con el salto en línea recta precalculado
    return destino_salto(origen, sobre) == destino;
}

bool puma_tiene_movimientos(const Tablero *t) {
    if (!t) return false;

    // 1. Localizar la posición del Puma
    int pos_puma = -1;
    for (int i = 0; i < NUM_CASILLAS; i++) {
        if (t->ocupante[i] == 0) {
            pos_puma = i;
            break;
        }
    }
    if (pos_puma == -1) return false; // Puma no encontrado

    // 2. Revisar cada vecino de la posición actual del puma
    for (int i = 0; i < g_grafo.num_vecinos[pos_puma]; i++) {
        int vec = g_grafo.vecinos[pos_puma][i];

        // Movimiento simple a casilla libre
        if (t->ocupante[vec] == VACIA) {
            return true;
        }

        // Intento de captura sobre perro
        if (t->ocupante[vec] >= 1 && t->ocupante[vec] <= NUM_PERROS) {
            int dst = destino_salto(pos_puma, vec);
            if (dst != -1 && t->ocupante[dst] == VACIA) {
                return true;
            }
        }
    }

    return false; // Puma completamente bloqueado/acorralado
}

static char simbolo_casilla(int ocupante) {
    if (ocupante == VACIA) return '.';
    if (ocupante == 0) return 'P';
    return 'D';
}

void imprimir_tablero(const Tablero *t) {
    if (!t) return;

    printf("\n=== TABLERO KOMIKAN === (Capturas Puma: %d/%d)\n",
           t->capturados, CAPTURAS_VICTORIA);
    printf("            ( %c )\n", simbolo_casilla(t->ocupante[28]));
    printf("           /  |   \\\n");
    printf("       ( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[25]),
           simbolo_casilla(t->ocupante[26]),
           simbolo_casilla(t->ocupante[27]));
    printf("      /  \\   /|\\   /  \\\n");
    printf("    ( %c )-( %c )-( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[0]), simbolo_casilla(t->ocupante[1]),
           simbolo_casilla(t->ocupante[2]), simbolo_casilla(t->ocupante[3]),
           simbolo_casilla(t->ocupante[4]));
    printf("    | \\ | / | \\ | / |\n");
    printf("    ( %c )-( %c )-( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[5]), simbolo_casilla(t->ocupante[6]),
           simbolo_casilla(t->ocupante[7]), simbolo_casilla(t->ocupante[8]),
           simbolo_casilla(t->ocupante[9]));
    printf("    | / | \\ | / | \\ |\n");
    printf("    ( %c )-( %c )-( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[10]), simbolo_casilla(t->ocupante[11]),
           simbolo_casilla(t->ocupante[12]), simbolo_casilla(t->ocupante[13]),
           simbolo_casilla(t->ocupante[14]));
    printf("    | \\ | / | \\ | / |\n");
    printf("    ( %c )-( %c )-( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[15]), simbolo_casilla(t->ocupante[16]),
           simbolo_casilla(t->ocupante[17]), simbolo_casilla(t->ocupante[18]),
           simbolo_casilla(t->ocupante[19]));
    printf("    | / | \\ | / | \\ |\n");
    printf("    ( %c )-( %c )-( %c )-( %c )-( %c )\n",
           simbolo_casilla(t->ocupante[20]), simbolo_casilla(t->ocupante[21]),
           simbolo_casilla(t->ocupante[22]), simbolo_casilla(t->ocupante[23]),
           simbolo_casilla(t->ocupante[24]));
    printf("===============================================\n\n");
}

