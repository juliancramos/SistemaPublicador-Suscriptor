#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DEFAULT_PIPE_PSC "/tmp/pipePSC"
#define DEFAULT_PIPE_SSC "/tmp/pipeSSC"
#define DEFAULT_FILE "noticias.txt"
#define DEFAULT_TIEMPO 5

void inicializarPipes(const char *pipePSC, const char *pipeSSC) {
    // Eliminar pipes existentes si es necesario
    unlink(pipePSC);
    unlink(pipeSSC);

    // Crear los pipes
    if (mkfifo(pipePSC, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear el pipe PSC");
            exit(EXIT_FAILURE);
        }
    }
    if (mkfifo(pipeSSC, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear el pipe SSC");
            exit(EXIT_FAILURE);
        }
    }
}


void publicar(const char *pipePSC, const char *archivo, int tiempo) {
    int fd = open(pipePSC, O_WRONLY);
    if (fd == -1) {
        perror("Error al abrir el pipe PSC para escritura");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        perror("Error al abrir el archivo de noticias");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        ssize_t bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("Error al escribir en el pipe PSC");
            close(fd);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        // Dormir el tiempo determinado, aunque podría mejorarse con control de flujo
        sleep(tiempo);
    }

    fclose(fp);
    close(fd);
}


void suscribir(const char *pipeSSC) {
    int fd = open(pipeSSC, O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el pipe SSC para lectura");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        printf("%s", buffer);
    }

    if (bytes_read == -1) {
        perror("Error al leer del pipe SSC");
    }

    close(fd);
}


int main(int argc, char *argv[]) {
    char *pipePSC = DEFAULT_PIPE_PSC;
    char *pipeSSC = DEFAULT_PIPE_SSC;
    char *file = DEFAULT_FILE;
    int tiempo = DEFAULT_TIEMPO;

    // Parsear argumentos (implementación detallada aquí)
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pipePSC = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            pipeSSC = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            file = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            tiempo = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Opción inválida: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    inicializarPipes(pipePSC, pipeSSC);

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo: publicador
        publicar(pipePSC, file, tiempo);
    } else if (pid > 0) {
        // Proceso padre: suscriptor
        suscribir(pipeSSC);
        wait(NULL); // Esperar a que el publicador termine

        // Eliminar pipes
        unlink(pipePSC);
        unlink(pipeSSC);
    } else {
        perror("Error en el fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}