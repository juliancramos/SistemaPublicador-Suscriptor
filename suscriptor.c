#include "suscriptor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

static volatile sig_atomic_t seguirEjecutando = 1;

void manejador_sigint(int sig) {
    seguirEjecutando = 0;
}

void inicializarSuscriptor(Suscriptor *suscriptor, const char *pipe) {
    printf("Inicializando suscriptor...\n");
    
    strncpy(suscriptor->pipeSSC, pipe, sizeof(suscriptor->pipeSSC) - 1);
    suscriptor->pipeSSC[sizeof(suscriptor->pipeSSC) - 1] = '\0';
    suscriptor->numTopicos = 0;
    
    struct stat st;
    if (stat(pipe, &st) == -1) {
        fprintf(stderr, "Error: El pipe %s no existe\n", pipe);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, manejador_sigint);
    printf("Suscriptor inicializado correctamente.\n");
}

void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void enviarSuscripciones(Suscriptor *suscriptor) {
    char topicosValidos[] = "AECPS";
    char entrada[100];
    int intentos = 0;
    const int MAX_INTENTOS = 3;

    do {
        if (intentos > 0) {
            printf("\nIntento %d de %d\n", intentos + 1, MAX_INTENTOS);
        }
        
        printf("Ingrese los tópicos a suscribirse (A:Arte, E:Espectáculos, C:Ciencia, P:Política, S:Sucesos)\n");
        printf("Ejemplo: A C P (separados por espacios): ");
        fflush(stdout);

        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            perror("Error al leer entrada");
            exit(EXIT_FAILURE);
        }

        // Limpiar la entrada
        size_t len = strlen(entrada);
        if (len > 0 && entrada[len-1] == '\n') {
            entrada[len-1] = '\0';
        }

        // Reiniciar contador de tópicos
        suscriptor->numTopicos = 0;

        // Procesar cada carácter de la entrada
        char *token = strtok(entrada, " \t");
        while (token != NULL && suscriptor->numTopicos < MAX_TOPICOS) {
            char topico = toupper(token[0]);
            if (strchr(topicosValidos, topico) != NULL) {
                // Verificar si ya está suscrito a este tópico
                int yaExiste = 0;
                for (int i = 0; i < suscriptor->numTopicos; i++) {
                    if (suscriptor->topicos[i][0] == topico) {
                        yaExiste = 1;
                        break;
                    }
                }
                
                if (!yaExiste) {
                    suscriptor->topicos[suscriptor->numTopicos][0] = topico;
                    suscriptor->topicos[suscriptor->numTopicos][1] = '\0';
                    suscriptor->numTopicos++;
                }
            }
            token = strtok(NULL, " \t");
        }

        intentos++;
        
        if (suscriptor->numTopicos == 0 && intentos < MAX_INTENTOS) {
            printf("Error: Debe seleccionar al menos un tópico válido.\n");
            continue;
        }
    } while (suscriptor->numTopicos == 0 && intentos < MAX_INTENTOS);

    if (suscriptor->numTopicos == 0) {
        fprintf(stderr, "Error: No se seleccionaron tópicos válidos después de %d intentos\n", MAX_INTENTOS);
        exit(EXIT_FAILURE);
    }

    printf("\nSuscrito exitosamente a los siguientes tópicos:\n");
    for (int i = 0; i < suscriptor->numTopicos; i++) {
        switch(suscriptor->topicos[i][0]) {
            case 'A': printf("- Arte\n"); break;
            case 'E': printf("- Espectáculos\n"); break;
            case 'C': printf("- Ciencia\n"); break;
            case 'P': printf("- Política\n"); break;
            case 'S': printf("- Sucesos\n"); break;
        }
    }
}

void recibirNoticias(Suscriptor *suscriptor) {
    printf("\nEsperando noticias...\n");
    printf("Presione Ctrl+C para salir\n\n");

    int fdPipe;
    fd_set readfds;
    struct timeval tv;
    
    while (seguirEjecutando) {
        fdPipe = open(suscriptor->pipeSSC, O_RDONLY | O_NONBLOCK);
        if (fdPipe < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error al abrir el pipe para lectura");
            exit(EXIT_FAILURE);
        }

        while (seguirEjecutando) {
            FD_ZERO(&readfds);
            FD_SET(fdPipe, &readfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int resultado = select(fdPipe + 1, &readfds, NULL, NULL, &tv);
            
            if (resultado > 0) {
                Noticia noticia;
                ssize_t bytesLeidos = read(fdPipe, &noticia, sizeof(Noticia));
                
                if (bytesLeidos == sizeof(Noticia)) {
                    for (int i = 0; i < suscriptor->numTopicos; i++) {
                        if (suscriptor->topicos[i][0] == noticia.tipo) {
                            time_t ahora = time(NULL);
                            struct tm *tiempo = localtime(&ahora);
                            printf("[%02d:%02d:%02d] [%c]: %s\n",
                                tiempo->tm_hour, tiempo->tm_min, tiempo->tm_sec,
                                noticia.tipo, noticia.texto);
                            break;
                        }
                    }
                } else if (bytesLeidos == 0) {
                    break;
                }
            } else if (resultado < 0 && errno != EINTR) {
                perror("Error en select");
                break;
            }
        }
        
        close(fdPipe);
        if (seguirEjecutando) {
            sleep(1);  // Esperar antes de reintentar
        }
    }
    
    printf("\nFinalizando suscriptor...\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-s") != 0) {
        fprintf(stderr, "Uso: %s -s pipeSSC\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Suscriptor suscriptor;
    inicializarSuscriptor(&suscriptor, argv[2]);
    enviarSuscripciones(&suscriptor);
    recibirNoticias(&suscriptor);

    return 0;
}