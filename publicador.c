#include "publicador.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_REINTENTOS 5
#define TIEMPO_ESPERA_BASE 1

static volatile sig_atomic_t seguirEjecutando = 1;

void manejador_sigint(int sig) {
    seguirEjecutando = 0;
}

void enviarNoticia(int fdPipe, const Noticia *noticia) {
    int intentos = 0;
    ssize_t bytesEscritos;

    do {
        bytesEscritos = write(fdPipe, noticia, sizeof(Noticia));
        if (bytesEscritos == sizeof(Noticia)) {
            printf("Noticia enviada [%c]: %s\n", noticia->tipo, noticia->texto);
            return;
        }
        
        if (bytesEscritos == -1 && errno != EAGAIN) {
            perror("Error al escribir en el pipe");
            return;
        }

        sleep(TIEMPO_ESPERA_BASE * (1 << intentos));
        intentos++;
    } while (intentos < MAX_REINTENTOS && seguirEjecutando);

    if (intentos >= MAX_REINTENTOS) {
        fprintf(stderr, "No se pudo enviar la noticia después de %d intentos\n", MAX_REINTENTOS);
    }
}

void procesarNoticias(const char *pipePSC, const char *archivo, int tiempoEspera) {
    signal(SIGINT, manejador_sigint);

    // Verificar que el pipe existe
    struct stat st;
    if (stat(pipePSC, &st) == -1) {
        fprintf(stderr, "El pipe %s no existe\n", pipePSC);
        exit(EXIT_FAILURE);
    }

    // Abrir archivo de noticias
    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        perror("Error al abrir el archivo de noticias");
        exit(EXIT_FAILURE);
    }

    // Abrir pipe con reintentos
    int fdPipe = -1;
    int intentos = 0;
    while (fdPipe == -1 && intentos < MAX_REINTENTOS && seguirEjecutando) {
        fdPipe = open(pipePSC, O_WRONLY | O_NONBLOCK);
        if (fdPipe == -1) {
            printf("Reintentando conexión al pipe PSC (%d/%d)...\n", intentos + 1, MAX_REINTENTOS);
            sleep(TIEMPO_ESPERA_BASE);
            intentos++;
        }
    }

    if (fdPipe == -1) {
        perror("Error al abrir el pipe PSC");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    char linea[100];
    while (fgets(linea, sizeof(linea), fp) && seguirEjecutando) {
        // Eliminar el salto de línea
        size_t len = strlen(linea);
        if (len > 0 && linea[len-1] == '\n') {
            linea[len-1] = '\0';
        }

        Noticia noticia = crearNoticia(linea);
        if (noticia.tipo != 'X') {
            enviarNoticia(fdPipe, &noticia);
            if (tiempoEspera > 0) {
                sleep(tiempoEspera);
            }
        } else {
            fprintf(stderr, "Error en línea: %s (Formato inválido)\n", linea);
        }
    }

    printf("Finalizando publicador...\n");
    fclose(fp);
    close(fdPipe);
}

int main(int argc, char *argv[]) {
    char *pipePSC = NULL;
    char *archivo = NULL;
    int tiempoEspera = 0;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            fprintf(stderr, "Faltan argumentos después de %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
        
        if (strcmp(argv[i], "-p") == 0) {
            pipePSC = argv[i + 1];
        } else if (strcmp(argv[i], "-f") == 0) {
            archivo = argv[i + 1];
        } else if (strcmp(argv[i], "-t") == 0) {
            tiempoEspera = atoi(argv[i + 1]);
            if (tiempoEspera < 0) {
                fprintf(stderr, "El tiempo de espera no puede ser negativo\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Opción desconocida: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (!pipePSC || !archivo) {
        fprintf(stderr, "Uso: %s -p pipePSC -f archivo [-t tiempoEspera]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    procesarNoticias(pipePSC, archivo, tiempoEspera);
    return 0;
}