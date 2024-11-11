#ifndef NOTICIAS_H
#define NOTICIAS_H

typedef struct {
    char tipo; // Tipo de noticia: A, E, C, P, S
    char texto[81]; // Texto de la noticia, máximo 80 caracteres + terminador
} Noticia;

// Funciones relacionadas con las noticias
int validarFormatoNoticia(const char *linea); // Valida el formato de una noticia
Noticia crearNoticia(const char *linea); // Crea una estructura Noticia a partir de una línea de texto

#endif // NOTICIAS_H
