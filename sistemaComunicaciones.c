#include "sistemaComunicaciones.h"
#include "noticia.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_SUSCRIPTORES 100
#define MAX_BUFFER 100
Suscripcion *cabeza = NULL; // Inicialización de la lista enlazada

// Estructura para almacenar las suscripciones (definida en el .h)

Suscripcion suscriptores[MAX_SUSCRIPTORES];
int numSuscriptores = 0;

// Variables globales para los nombres de los pipes
char pipePSC[100];
char pipeSSC[100];
int tiempoEspera;

// Función para obtener el nombre del pipe de un suscriptor
char* obtenerPipeSuscriptor(int id) {
    static char pipeName[100];
    sprintf(pipeName, "pipe_suscriptor_%d", id);
    return pipeName;
}

void inicializarSistema(const char *pipePSCPath, const char *pipeSSCPath, int espera) {
    strncpy(pipePSC, pipePSCPath, sizeof(pipePSC) - 1);
    strncpy(pipeSSC, pipeSSCPath, sizeof(pipeSSC) - 1);
    tiempoEspera = espera;

    if (mkfifo(pipePSC, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear el pipe PSC");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(pipeSSC, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear el pipe SSC");
        exit(EXIT_FAILURE);
    }
}

void procesarNoticias() {
    int fdPipePSC = open(pipePSC, O_RDONLY);
    char buffer[MAX_BUFFER];
    while (read(fdPipePSC, buffer, sizeof(buffer)) > 0) {
        distribuirNoticias(buffer);
    }
    close(fdPipePSC);
}

void manejarSuscripciones() {
    int fdPipeSSC = open(pipeSSC, O_RDONLY);
    char buffer[MAX_BUFFER];
    while (read(fdPipeSSC, buffer, sizeof(buffer)) > 0) {
        Suscripcion nuevaSuscripcion;
        if (strlen(buffer) <= MAX_TOPICOS && strspn(buffer, "AECPS") == strlen(buffer)) {
            nuevaSuscripcion.id = numSuscriptores;
            strncpy(nuevaSuscripcion.topicos, buffer, MAX_TOPICOS);
            if (numSuscriptores < MAX_SUSCRIPTORES) {
                suscriptores[numSuscriptores++] = nuevaSuscripcion;
                char pipeName[100];
                sprintf(pipeName, "pipe_suscriptor_%d", nuevaSuscripcion.id);
                mkfifo(pipeName, 0666);
            } else {
                printf("Límite de suscriptores alcanzado.\n");
            }
        } else {
            printf("Suscripción inválida.\n");
        }
    }
    close(fdPipeSSC);
}

void distribuirNoticias(const char *noticia) {
    char tipoNoticia = noticia[0];

    for (int i = 0; i < numSuscriptores; i++) {
        for (size_t j = 0; j < strlen(suscriptores[i].topicos); j++) {
            if (suscriptores[i].topicos[j] == tipoNoticia) {
                char* pipeSuscriptor = obtenerPipeSuscriptor(suscriptores[i].id);
                int fdPipeSuscriptor = open(pipeSuscriptor, O_WRONLY);

                if (fdPipeSuscriptor != -1) {
                    if (write(fdPipeSuscriptor, noticia, strlen(noticia)) != (ssize_t)strlen(noticia)) {
                        perror("Error al escribir en el pipe");

                        // Cerrar el pipe
                        close(fdPipeSuscriptor);

                        // Registrar el error en un log
                        FILE *log = fopen("error_log.txt", "a");
                        if (log != NULL) {
                            fprintf(log, "Error al distribuir noticia: %s\n", strerror(errno));
                            fclose(log);
                        } else {
                            perror("Error al abrir el archivo de log");
                        }

                        // Intentar reescribir la noticia más tarde (opcional)
                        // ... (implementar lógica para reintentar)

                        // Notificar al administrador del sistema (opcional)
                        // ... (implementar lógica para enviar una notificación)
                    }
                    close(fdPipeSuscriptor);
                } else {
                    perror("Error al abrir el pipe");
                    // Aquí puedes agregar más acciones de manejo de errores, como:
                    // - Eliminar al suscriptor de la lista
                    // - Intentar volver a crear el pipe
                }
            }
        }
    }
}

void agregarSuscripcion(int id, const char *topicos) {
    // Validar si el ID ya existe
    Suscripcion* actual = cabeza;
    while (actual != NULL) {
        if (actual->id == id) {
            printf("El ID de suscripción ya existe.\n");
            return;
        }
        actual = actual->siguiente;
    }

    // Validar los tópicos
    if (strlen(topicos) > MAX_TOPICOS - 1 || strspn(topicos, "AECPS") != strlen(topicos)) {
        printf("Suscripción inválida.\n");
        return;
    }

    // Crear un nuevo nodo para la lista enlazada
    Suscripcion* nuevoNodo = (Suscripcion*)malloc(sizeof(Suscripcion));
    nuevoNodo->id = id;
    strncpy(nuevoNodo->topicos, topicos, MAX_TOPICOS);
    nuevoNodo->siguiente = cabeza;
    cabeza = nuevoNodo;

    // Crear un pipe para el nuevo suscriptor
    char pipeName[100];
    sprintf(pipeName, "pipe_suscriptor_%d", id);
    if (mkfifo(pipeName, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear el pipe del suscriptor");
        // Liberar la memoria del nodo creado
        free(nuevoNodo);
        return;
    }

    printf("Nueva suscripción registrada: %s\n", topicos);
}

void eliminarSuscripcion(int id) {
    Suscripcion *actual = cabeza;
    Suscripcion *anterior = NULL;

    // Recorremos la lista enlazada hasta encontrar el nodo con el id a eliminar
    while (actual != NULL) {
        if (actual->id == id) {
            // Si el nodo a eliminar es el primero de la lista
            if (anterior == NULL) {
                cabeza = actual->siguiente; // Actualizamos el puntero a la cabeza
            } else { // Si el nodo a eliminar está en medio o al final de la lista
                anterior->siguiente = actual->siguiente; // Enlazamos el nodo anterior con el siguiente
            }

            // Eliminamos el pipe asociado al suscriptor
            char pipeName[100];
            sprintf(pipeName, "pipe_suscriptor_%d", id);
            unlink(pipeName);

            // Liberamos la memoria del nodo eliminado
            free(actual);

            printf("Suscripción eliminada con éxito.\n");
            return;
        }

        anterior = actual; // Avanzamos al siguiente nodo
        actual = actual->siguiente;
    }

    // Si llegamos al final de la lista sin encontrar el nodo, el suscriptor no existe
    printf("Suscriptor no encontrado.\n");
}