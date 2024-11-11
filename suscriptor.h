#ifndef SUSCRIPTOR_H
#define SUSCRIPTOR_H

#include "noticia.h"

#define MAX_TOPICOS 5
#define MAX_TOPICO_LENGTH 2
#define MAX_MENSAJE_LENGTH 81

typedef struct {
    char pipeSSC[256];
    char topicos[MAX_TOPICOS][MAX_TOPICO_LENGTH];
    int numTopicos;
} Suscriptor;

void inicializarSuscriptor(Suscriptor *suscriptor, const char *pipe);
void enviarSuscripciones(Suscriptor *suscriptor);
void recibirNoticias(Suscriptor *suscriptor);

#endif // SUSCRIPTOR_H