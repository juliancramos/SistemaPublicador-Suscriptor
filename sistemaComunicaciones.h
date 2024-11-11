#ifndef SISTEMACOMUNICACIONES_H
#define SISTEMACOMUNICACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_SUSCRIPTORES 100

// Forward declaration of MAX_TOPICOS
#define MAX_TOPICOS 6

typedef struct Suscripcion {
    int id;
    char topicos[MAX_TOPICOS];
    struct Suscripcion *siguiente;
} Suscripcion;

extern Suscripcion *cabeza;

void inicializarSistema(const char *pipePSCPath, const char *pipeSSCPath, int espera);
void procesarNoticias();
void manejarSuscripciones();
void distribuirNoticias(const char *noticia);
void agregarSuscripcion(int id, const char *topicos);
void eliminarSuscripcion(int id);

#endif