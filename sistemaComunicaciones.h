// sistemaComunicaciones.h
#ifndef SISTEMA_COMUNICACIONES_H
#define SISTEMA_COMUNICACIONES_H

#include "noticia.h"

void inicializarSistema(const char *pipePSC, const char *pipeSSC, int tiempoEspera);
void gestionarComunicaciones();
void finalizarSistema();

#endif // SISTEMA_COMUNICACIONES_H