#include "noticia.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int validarFormatoNoticia(const char *linea) {
    if (strlen(linea) < 4 || linea[1] != ':') {
        return 0; // Formato inválido
    }

    char tipo = linea[0];
    if (tipo != 'A' && tipo != 'E' && tipo != 'C' && tipo != 'P' && tipo != 'S') {
        return 0; // Tipo de noticia inválido
    }

    if (linea[strlen(linea) - 1] != '.') {
        return 0; // Debe terminar con un punto
    }

    return 1; // Formato válido
}

Noticia crearNoticia(const char *linea) {
    Noticia noticia;
    if (validarFormatoNoticia(linea)) {
        noticia.tipo = linea[0];
        strncpy(noticia.texto, linea + 2, sizeof(noticia.texto) - 1);
        noticia.texto[sizeof(noticia.texto) - 1] = '\0'; // Asegura el fin de la cadena
    } else {
        noticia.tipo = '\0'; // Indica un tipo inválido
        strcpy(noticia.texto, "Formato de noticia inválido.");
    }
    return noticia;
}
