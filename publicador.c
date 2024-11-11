#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>  // Para obtener la hora actual
#include <errno.h>

#define MAX_REINTENTOS 3
#define TIEMPO_ESPERA 5  // En segundos

void enviarNoticia(int fdPipeSuscriptor, const char *noticia, int tamanoNoticia);

void enviarNoticias(const char *pipePSC, const char *archivo) {
    int fdPipe = open(pipePSC, O_WRONLY);
    if (fdPipe == -1) {
        perror("Error al abrir el pipe PSC");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        perror("Error al abrir el archivo de noticias");
        close(fdPipe);
        exit(EXIT_FAILURE);
    }

    char noticia[100];
    while (fgets(noticia, sizeof(noticia), fp)) {
        // Eliminar el salto de línea si lo hay
        noticia[strcspn(noticia, "\n")] = '\0';

        int tamanoNoticia = strlen(noticia);
        enviarNoticia(fdPipe, noticia, tamanoNoticia);
    }

    fclose(fp);
    close(fdPipe);
}

void enviarNoticia(int fdPipeSuscriptor, const char *noticia, int tamanoNoticia) {
    int intentos = 0;
    while (intentos < MAX_REINTENTOS) {
        if (write(fdPipeSuscriptor, noticia, tamanoNoticia) == tamanoNoticia) {
            return;  // Éxito al escribir
        }

        perror("Error al escribir en el pipe");

        // Registrar el error en un log
        FILE *log = fopen("error.log", "a");
        if (log != NULL) {
            time_t now = time(NULL);
            fprintf(log, "[%s] Error al escribir en el pipe para el suscriptor: %s\n", ctime(&now), strerror(errno));
            fclose(log);
        }

        // Esperar antes de reintentar
        sleep(TIEMPO_ESPERA);
        intentos++;
    }

    // Si se agotan los intentos, cerrar el pipe y notificar al administrador
    close(fdPipeSuscriptor);
    // Aquí puedes implementar la lógica para notificar al administrador (por ejemplo, enviar un correo electrónico)
    printf("Se agotaron los intentos de reescritura. Notificar al administrador.\n");
}