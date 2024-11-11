#ifndef SUSCRIPTOR_H
#define SUSCRIPTOR_H

#define MAX_TOPICOS 5
#define MAX_TOPICO_LENGTH 10

#include <string.h> // Para usar funciones de cadenas

// Definición de la estructura Suscriptor
typedef struct {
    char pipeSSC[50];
    char topicos[MAX_TOPICOS][MAX_TOPICO_LENGTH];
    int numTopicos;
} Suscriptor;

// Prototipos de funciones
void inicializarSuscriptor(Suscriptor *suscriptor, const char *pipe);
void enviarSuscripciones(Suscriptor *suscriptor);
void recibirNoticias(Suscriptor *suscriptor);

#endif // SUSCRIPTOR_H