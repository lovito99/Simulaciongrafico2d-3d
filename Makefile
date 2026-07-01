CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11
LDFLAGS = -lGLEW -lglfw -lGL -lm

TARGET = entrega2_transformaciones
SRC    = main.c escena.c algoritmos.c transformaciones.c render_escena.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
