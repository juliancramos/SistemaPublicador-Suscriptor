// publicador.h
#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include "noticia.h"

void enviarNoticia(int fdPipeSuscriptor, const Noticia *noticia);
void enviarNoticias(const char *pipePSC, const char *archivo, int tiempoEspera);

#endif // PUBLICADOR_H