#include "sistemaComunicaciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define MAX_BUFFER_NOTICIAS 100

typedef struct {
    Noticia noticias[MAX_BUFFER_NOTICIAS];
    int inicio;
    int fin;
    int contador;
} BufferCircular;

static char pipePublicador[256];
static char pipeSuscriptor[256];
static int tiempoForward;
static volatile sig_atomic_t seguirEjecutando = 1;
static BufferCircular buffer;

void manejador_sigint(int sig) {
    seguirEjecutando = 0;
}

void inicializarBuffer() {
    buffer.inicio = 0;
    buffer.fin = 0;
    buffer.contador = 0;
}

void agregarNoticia(const Noticia *noticia) {
    if (buffer.contador < MAX_BUFFER_NOTICIAS) {
        buffer.noticias[buffer.fin] = *noticia;
        buffer.fin = (buffer.fin + 1) % MAX_BUFFER_NOTICIAS;
        buffer.contador++;
    }
}

int obtenerNoticia(Noticia *noticia) {
    if (buffer.contador > 0) {
        *noticia = buffer.noticias[buffer.inicio];
        buffer.inicio = (buffer.inicio + 1) % MAX_BUFFER_NOTICIAS;
        buffer.contador--;
        return 1;
    }
    return 0;
}

void inicializarSistema(const char *pipePSC, const char *pipeSSC, int tiempoEspera) {
    printf("Iniciando sistema de comunicaciones...\n");
    
    strncpy(pipePublicador, pipePSC, sizeof(pipePublicador) - 1);
    strncpy(pipeSuscriptor, pipeSSC, sizeof(pipeSuscriptor) - 1);
    tiempoForward = tiempoEspera;
    
    // Eliminar pipes si existen
    unlink(pipePublicador);
    unlink(pipeSuscriptor);

    // Crear pipes nominales con manejo de errores
    if (mkfifo(pipePublicador, 0666) == -1) {
        fprintf(stderr, "Error creando pipe PSC: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (mkfifo(pipeSuscriptor, 0666) == -1) {
        fprintf(stderr, "Error creando pipe SSC: %s\n", strerror(errno));
        unlink(pipePublicador);
        exit(EXIT_FAILURE);
    }

    inicializarBuffer();
    printf("Sistema inicializado correctamente.\n");
    printf("Pipe PSC: %s\nPipe SSC: %s\n", pipePublicador, pipeSuscriptor);
    printf("Tiempo de reenvío: %d segundos\n", tiempoEspera);
}

void reenviarNoticiasASuscriptores() {
    int fdSSC = open(pipeSuscriptor, O_WRONLY | O_NONBLOCK);
    if (fdSSC >= 0) {
        Noticia noticia;
        while (obtenerNoticia(&noticia)) {
            ssize_t bytesEscritos = write(fdSSC, &noticia, sizeof(Noticia));
            if (bytesEscritos == sizeof(Noticia)) {
                printf("Reenviada noticia [%c]: %s\n", noticia.tipo, noticia.texto);
                if (tiempoForward > 0) {
                    sleep(tiempoForward);
                }
            }
        }
        close(fdSSC);
    }
}

void gestionarComunicaciones() {
    signal(SIGINT, manejador_sigint);
    printf("Iniciando gestión de comunicaciones...\n");
    
    int fdPSC;
    fd_set readfds;
    struct timeval tv;
    
    while (seguirEjecutando) {
        fdPSC = open(pipePublicador, O_RDONLY | O_NONBLOCK);
        if (fdPSC < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error al abrir pipe PSC");
            break;
        }

        while (seguirEjecutando) {
            FD_ZERO(&readfds);
            FD_SET(fdPSC, &readfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int resultado = select(fdPSC + 1, &readfds, NULL, NULL, &tv);
            
            if (resultado > 0) {
                Noticia noticia;
                ssize_t bytesLeidos = read(fdPSC, &noticia, sizeof(Noticia));
                
                if (bytesLeidos == sizeof(Noticia)) {
                    printf("Recibida noticia tipo %c: %s\n", noticia.tipo, noticia.texto);
                    agregarNoticia(&noticia);
                    reenviarNoticiasASuscriptores();
                } else if (bytesLeidos == 0) {
                    printf("Pipe cerrado por el publicador. Esperando nuevas conexiones...\n");
                    break;
                }
            } else if (resultado == 0) {
                // Timeout - verificar si hay noticias pendientes para reenviar
                reenviarNoticiasASuscriptores();
            } else if (errno != EINTR) {
                perror("Error en select");
                break;
            }
        }
        
        close(fdPSC);
    }
}

void finalizarSistema() {
    printf("\nFinalizando sistema de comunicaciones...\n");
    unlink(pipePublicador);
    unlink(pipeSuscriptor);
    printf("Sistema finalizado correctamente.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 7 || strcmp(argv[1], "-p") != 0 || 
        strcmp(argv[3], "-s") != 0 || strcmp(argv[5], "-t") != 0) {
        fprintf(stderr, "Uso: %s -p pipePSC -s pipeSSC -t timeF\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int tiempoEspera = atoi(argv[6]);
    if (tiempoEspera < 0) {
        fprintf(stderr, "El tiempo de espera no puede ser negativo\n");
        exit(EXIT_FAILURE);
    }

    inicializarSistema(argv[2], argv[4], tiempoEspera);
    gestionarComunicaciones();
    finalizarSistema();
    return 0;
}