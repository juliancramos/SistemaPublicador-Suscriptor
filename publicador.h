#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include "noticia.h"

// Estructura para un publicador
typedef struct {
    char pipePSC[100];
    char filePath[100];
    int tiempoEntreNoticias;
} Publicador;

// Función para inicializar un publicador
void inicializarPublicador(Publicador *publicador, const char *pipe, const char *file, int tiempo);

// Función para enviar noticias al Sistema de Comunicación
void enviarNoticias(Publicador *publicador);

// Función para ejecutar el proceso del publicador
void publicador(const char *pipePSC, const char *file, int tiempo);

#endif // PUBLICADOR_H