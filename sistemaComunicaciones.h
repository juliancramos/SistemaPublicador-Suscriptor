#ifndef SISTEMA_COMUNICACIONES_H
#define SISTEMA_COMUNICACIONES_H

#include "noticia.h"
#include "suscriptor.h"

#define MAX_NOTICIAS_BUFFER 100

typedef struct {
    NoticiaBuffer noticias[MAX_NOTICIAS_BUFFER];
    int inicio;
    int fin;
    int contador;
} BufferCircular;

extern int NUM_SUSCRIPTORES;
extern Suscriptor suscriptores[]; // Arreglo de suscriptores

void inicializarSistema(const char *pipePSC, const char *pipeSSC, int tiempoEspera);
void gestionarComunicaciones();
void finalizarSistema();
void agregarNoticia(const Noticia *noticia);
void reenviarNoticiasASuscriptores();

#endif // SISTEMA_COMUNICACIONES_H