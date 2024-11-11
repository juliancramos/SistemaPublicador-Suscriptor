#include "suscriptor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAX_TOPICOS 5
#define MAX_TOPICO_LENGTH 10



void inicializarSuscriptor(Suscriptor *suscriptor, const char *pipe) {
    strncpy(suscriptor->pipeSSC, pipe, sizeof(suscriptor->pipeSSC) - 1);
    suscriptor->pipeSSC[sizeof(suscriptor->pipeSSC) - 1] = '\0';
    suscriptor->numTopicos = 0;
}

void enviarSuscripciones(Suscriptor *suscriptor) {
    printf("Ingrese los tópicos a los que desea suscribirse (A, E, C, P, S): ");

    char topicosIngresados[MAX_TOPICOS + 1];
    if (scanf("%5s", topicosIngresados) != 1) {
        perror("Error al leer los tópicos");
        exit(EXIT_FAILURE);
    }

    // Validar la longitud total de los tópicos
    if (strlen(topicosIngresados) > MAX_TOPICOS) {
        printf("Demasiados tópicos ingresados.\n");
        return;
    }

    // Validar los tópicos ingresados y copiarlos a la estructura
    for (size_t i = 0; i < strlen(topicosIngresados); ++i) {
        if (strchr("AECPS", topicosIngresados[i]) == NULL) {
            printf("Tópico inválido: %c\n", topicosIngresados[i]);
            continue; // Saltar al siguiente tópico
        }
        suscriptor->topicos[suscriptor->numTopicos][0] = topicosIngresados[i];
        suscriptor->numTopicos++;
    }

    // Abrir el pipe nominal para escritura
    int fdPipe = open(suscriptor->pipeSSC, O_WRONLY);
    if (fdPipe < 0) {
        perror("Error al abrir el pipe nominal");
        exit(EXIT_FAILURE);
    }

    // Enviar los tópicos al Sistema de Comunicación
    if (write(fdPipe, suscriptor->topicos, sizeof(suscriptor->topicos)) != sizeof(suscriptor->topicos)) {
        perror("Error al escribir en el pipe");
        close(fdPipe);
        exit(EXIT_FAILURE);
    }

    close(fdPipe);
    printf("Suscripción enviada: ");
    for (int i = 0; i < suscriptor->numTopicos; ++i) {
        printf("%c", suscriptor->topicos[i][0]);
    }
    printf("\n");
}

void recibirNoticias(Suscriptor *suscriptor) {
    int fdPipe = open(suscriptor->pipeSSC, O_RDONLY);
    if (fdPipe < 0) {
        perror("Error al abrir el pipe nominal");
        exit(EXIT_FAILURE);
    }

    char buffer[81];
    while (1) {
        ssize_t bytesLeidos = read(fdPipe, buffer, sizeof(buffer) - 1);
        if (bytesLeidos <= 0) {
            if (bytesLeidos == 0) {
                printf("El pipe se cerró.\n");
            } else {
                perror("Error al leer del pipe");
            }
            break;
        }

        buffer[bytesLeidos] = '\0';
        printf("Noticia recibida: %s\n", buffer);
    }

    close(fdPipe);
}