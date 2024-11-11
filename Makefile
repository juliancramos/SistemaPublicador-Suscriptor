# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O2
SOURCES = main.c publicador.c sistemaComunicaciones.c suscriptor.c noticia.c
OBJECTS = $(SOURCES:.c=.o)
EXEC = sistema publicador suscriptor

# Regla por defecto
all: $(EXEC)

# Regla para compilar
$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Regla para compilar archivos .o
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Limpiar los archivos generados
clean:
	rm -f $(OBJECTS) $(EXEC)
