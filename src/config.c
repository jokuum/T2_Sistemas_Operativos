#include "komikan.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

static const int PERROS_DEFECTO [NUM_PERROS] = 
    {10, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

void config_por_defecto(Config *c) {
    c->retardo_min_ms = 50;
    c->retardo_max_ms = 300;
    c->semilla = (unsigned int) time(NULL);
    c->pos_puma = 27;
    memcpy(c->pos_perros, PERROS_DEFECTO, sizeof(PERROS_DEFECTO));
    c->prob_fallo = 0.0;
}

bool config_cargar(Config *c, const char *ruta) {
    config_por_defecto(c);
    if (ruta == NULL) return true;          /* sin archivo: valores por defecto */

    FILE *f = fopen(ruta, "r");
    if (f == NULL) {
        perror(ruta);
        return false;
    }

    char linea[256];
    int num_linea = 0;
    bool ok = true;

    while (fgets(linea, sizeof(linea), f) != NULL) {
        num_linea++;

        char clave[32], valor[200];
        char primero;
        /* Línea vacía o comentario: se ignora */
        if (sscanf(linea, " %c", &primero) != 1 || primero == '#') continue;

        /* Separar "clave = valor" */
        if (sscanf(linea, " %31[^= ] = %199[^\n]", clave, valor) != 2) {
            fprintf(stderr, "%s:%d: línea mal formada\n", ruta, num_linea);
            ok = false;
            break;
        }

        int *p = c->pos_perros;
        int leidos;
        if (strcmp(clave, "retardo_min") == 0) {
            leidos = sscanf(valor, "%d", &c->retardo_min_ms);
        } else if (strcmp(clave, "retardo_max") == 0) {
            leidos = sscanf(valor, "%d", &c->retardo_max_ms);
        } else if (strcmp(clave, "semilla") == 0) {
            leidos = sscanf(valor, "%u", &c->semilla);
        } else if (strcmp(clave, "puma") == 0) {
            leidos = sscanf(valor, "%d", &c->pos_puma);
        } else if (strcmp(clave, "prob_fallo") == 0) {
            leidos = sscanf(valor, "%lf", &c->prob_fallo);
        } else if (strcmp(clave, "perros") == 0) {
            leidos = sscanf(valor, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                            &p[0], &p[1], &p[2], &p[3], &p[4], &p[5],
                            &p[6], &p[7], &p[8], &p[9], &p[10], &p[11]);
            leidos = (leidos == NUM_PERROS);   /* tienen que ser los 12 */
        } else {
            leidos = 0;                         /* clave desconocida */
        }

        if (leidos != 1) {
            fprintf(stderr, "%s:%d: valor inválido para '%s'\n", ruta, num_linea, clave);
            ok = false;
            break;
        }
    }

    fclose(f);
    return ok && config_validar(c);
}

bool config_validar(const Config *c) {
    //1. Retardos_ no negativos y min <= max
    if (c->retardo_min_ms < 0 || c->retardo_max_ms < c->retardo_min_ms ||
        c->retardo_max_ms > RETARDO_MAX_PERMITIDO) {
        fprintf(stderr, "config: retardos invalidos (min=%d, max=%d)\n",
                c->retardo_min_ms, c->retardo_max_ms);
        return false;
    }

    //2. probabilidad de fallo entre 0 y 1
    if (c->prob_fallo < 0.0 || c->prob_fallo > 1.0) {
        fprintf(stderr, "config: prob_fallo debe estar entre 0 y 1\n");
        return false;
    }

    //3. posiciones: dentro del tablero y sin repetirse
    bool ocupada[NUM_CASILLAS] = {false};

    if (c->pos_puma < 0 || c->pos_puma >= NUM_CASILLAS) {
        fprintf(stderr, "config: posición del puma fuera de rango (%d)\n", c->pos_puma);
        return false;
    }
    ocupada[c->pos_puma] = true;

    for (int i = 0; i < NUM_PERROS; i++) {
        int pos = c->pos_perros[i];
        if (pos < 0 || pos >= NUM_CASILLAS || ocupada[pos]) {
            fprintf(stderr, "config: posición de perro inválida o repetida (%d)\n", pos);
            return false;
        }
        ocupada[pos] = true;
    }

    return true;
}

void config_imprimir(const Config *c) {
    char perros[64];
    int pos = 0;
    for (int i = 0; i < NUM_PERROS; i++) {
        pos += snprintf(perros + pos, sizeof(perros) - pos, "%d%s",
                        c->pos_perros[i], i + 1 < NUM_PERROS ? "," : "");
    }
    log_msg(-1, "Config: retardo=[%d,%d] ms, semilla=%u, prob_fallo=%.3f, puma=%d, perros=%s",
            c->retardo_min_ms, c->retardo_max_ms, c->semilla, c->prob_fallo,
            c->pos_puma, perros);
}
