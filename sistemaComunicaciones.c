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
    if (buffer.contador < MAX_NOTICIAS_BUFFER) {
        buffer.noticias[buffer.fin].tipo = noticia->tipo;
        strncpy(buffer.noticias[buffer.fin].texto, noticia->texto, sizeof(buffer.noticias[buffer.fin].texto) - 1);
        buffer.noticias[buffer.fin].texto[sizeof(buffer.noticias[buffer.fin].texto) - 1] = '\0';
        buffer.noticias[buffer.fin].enviada = 0;  // Nueva noticia, no enviada
        buffer.fin = (buffer.fin + 1) % MAX_NOTICIAS_BUFFER;
        buffer.contador++;
    } else {
        // Si el buffer está lleno, hacer espacio eliminando la noticia más antigua
        buffer.inicio = (buffer.inicio + 1) % MAX_NOTICIAS_BUFFER;
        buffer.contador--;
        // Y luego agregar la nueva noticia
        buffer.noticias[buffer.fin].tipo = noticia->tipo;
        strncpy(buffer.noticias[buffer.fin].texto, noticia->texto, sizeof(buffer.noticias[buffer.fin].texto) - 1);
        buffer.noticias[buffer.fin].texto[sizeof(buffer.noticias[buffer.fin].texto) - 1] = '\0';
        buffer.noticias[buffer.fin].enviada = 0;
        buffer.fin = (buffer.fin + 1) % MAX_NOTICIAS_BUFFER;
        buffer.contador++;
    }
}

int obtenerNoticia(Noticia *noticia) {
    if (buffer.contador > 0) {
        noticiabufferToNoticia(&buffer.noticias[buffer.inicio], noticia);
        buffer.inicio = (buffer.inicio + 1) % MAX_NOTICIAS_BUFFER;
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
    static int intentosEnvio = 0;
    const int MAX_INTENTOS_POR_NOTICIA = 5;
    
    // Abrir el pipe para escritura
    int fdSSC = open(pipeSuscriptor, O_WRONLY | O_NONBLOCK);
    if (fdSSC < 0) {
        return;  // Si no hay suscriptores, simplemente retornamos
    }

    // Recorrer el buffer y enviar las noticias no enviadas
    int i = buffer.inicio;
    int noticiasRestantes = buffer.contador;
    
    while (noticiasRestantes > 0 && seguirEjecutando) {
        if (!buffer.noticias[i].enviada) {
            Noticia noticia;
            noticiabufferToNoticia(&buffer.noticias[i], &noticia);
            ssize_t bytesEscritos = write(fdSSC, &noticia, sizeof(Noticia));
            
            if (bytesEscritos == sizeof(Noticia)) {
                printf("Noticia reenviada [%c]: %s\n", 
                       buffer.noticias[i].tipo,
                        buffer.noticias[i].texto);
                       
                buffer.noticias[i].enviada = 1;  // Marcar como enviada
                intentosEnvio = 0;
                
                if (tiempoForward > 0) {
                    sleep(tiempoForward);
                }
            } else {
                intentosEnvio++;
                if (intentosEnvio >= MAX_INTENTOS_POR_NOTICIA) {
                    buffer.noticias[i].enviada = 1;  // Marcar como enviada después de máximos intentos
                    intentosEnvio = 0;
                }
                break;  // Salir del bucle si hay error de escritura
            }
        }
        
        i = (i + 1) % MAX_NOTICIAS_BUFFER;
        noticiasRestantes--;
    }
    
    // Limpiar noticias enviadas del buffer
    while (buffer.contador > 0 && buffer.noticias[buffer.inicio].enviada) {
        buffer.inicio = (buffer.inicio + 1) % MAX_NOTICIAS_BUFFER;
        buffer.contador--;
    }
    
    close(fdSSC);
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
            if (errno == EINTR) continue;
            perror("Error al abrir pipe PSC");
            sleep(1);
            continue;
        }

        while (seguirEjecutando) {
            FD_ZERO(&readfds);
            FD_SET(fdPSC, &readfds);
            tv.tv_sec = 0;  // Reducir el tiempo de espera
            tv.tv_usec = 100000;  // 100ms

            int resultado = select(fdPSC + 1, &readfds, NULL, NULL, &tv);
            
            if (resultado > 0) {
                Noticia noticia;
                ssize_t bytesLeidos = read(fdPSC, &noticia, sizeof(Noticia));
                
                if (bytesLeidos == sizeof(Noticia)) {
                    printf("Recibida noticia tipo %c: %s\n", noticia.tipo, noticia.texto);
                    agregarNoticia(&noticia);
                } else if (bytesLeidos == 0) {
                    break;
                }
            }
            
            // Reenviar más frecuentemente
            reenviarNoticiasASuscriptores();
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