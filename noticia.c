#include "noticia.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int validarFormatoNoticia(const char *linea) {
    printf("Línea recibida: '%s'\n", linea);

    // Crear una copia para manipular la línea
    char copia[100];
    strncpy(copia, linea, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    // Eliminar espacios al inicio y al final
    char *inicio = copia;
    while (isspace(*inicio)) inicio++;
    char *fin = inicio + strlen(inicio) - 1;
    while (fin > inicio && isspace(*fin)) fin--;
    *(fin + 1) = '\0'; // Termina la cadena limpiada

    printf("Línea después de limpiar espacios: '%s'\n", inicio);

    // Verificar longitud mínima (tipo:texto.)
    if (strlen(inicio) < 4) {
        printf("Error: Longitud mínima no cumplida.\n");
        return 0;
    }

    // Verificar formato "X: texto."
    if (inicio[1] != ':') {
        printf("Error: Formato de tipo de noticia incorrecto.\n");
        return 0;
    }

    // Verificar que el primer carácter es uno de los tipos permitidos
    char tipo = toupper(inicio[0]);
    if (strchr("AECPS", tipo) == NULL) {
        printf("Error: Tipo de noticia no válido.\n");
        return 0;
    }

    // Verificar que la línea termine en un punto
    if (inicio[strlen(inicio) - 1] != '.') {
        printf("Error: La línea no termina en punto.\n");
        return 0;
    }

    // Verificar longitud máxima de 80 caracteres para el contenido de la noticia
    if (strlen(inicio) > 83) {
        printf("Error: Longitud máxima excedida.\n");
        return 0;
    }

    return 1;
}


Noticia crearNoticia(const char *linea) {
    Noticia noticia;
    memset(&noticia, 0, sizeof(Noticia)); // Inicializar estructura

    if (!validarFormatoNoticia(linea)) {
        noticia.tipo = 'X'; // Marca de error
        strncpy(noticia.texto, "Error: Formato inválido", sizeof(noticia.texto) - 1);
        return noticia;
    }

    noticia.tipo = toupper(linea[0]);
    // Copiar el texto después de "X: "
    strncpy(noticia.texto, linea + 3, sizeof(noticia.texto) - 1);
    noticia.texto[sizeof(noticia.texto) - 1] = '\0';

    return noticia;
}

void noticiabufferToNoticia(const NoticiaBuffer *noticiabuffer, Noticia *noticia) {
    noticia->tipo = noticiabuffer->tipo;
    strncpy(noticia->texto, noticiabuffer->texto, sizeof(noticia->texto) - 1);
    noticia->texto[sizeof(noticia->texto) - 1] = '\0';
}
