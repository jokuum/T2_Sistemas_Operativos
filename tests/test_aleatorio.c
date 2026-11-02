#include "komikan.h"
#include <stdlib.h>

int main(void) {
    for (int rep = 0; rep < 2; rep++) {
        unsigned int puma  = semilla_hilo(42, 0, 0);
        unsigned int perro = semilla_hilo(42, 0, 1);
        printf("Ejecución %d  puma:", rep + 1);
        for (int i = 0; i < 5; i++) printf(" %d", aleatorio_rango(&puma, 50, 300));
        printf("   perro 1:");
        for (int i = 0; i < 5; i++) printf(" %d", aleatorio_rango(&perro, 50, 300));
        printf("\n");
    }
    dormir_ms(100);
    printf("Dormí 100 ms\n");
    return 0;
}