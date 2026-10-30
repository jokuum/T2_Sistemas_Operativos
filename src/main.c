#include "komikan.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s N M [config.txt]\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("Torneo Komikan — N=%s, M=%s, config=%s\n",
           argv[1], argv[2], argc == 4 ? argv[3] : "(default)");
    return EXIT_SUCCESS;
}