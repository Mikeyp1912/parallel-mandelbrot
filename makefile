CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -pthread
TARGET = mandelbrot

SRC = src/main.c \
      src/mandelbrot_core.c \
      src/mandelbrot_colour.c \
      src/mandelbrot_output.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) -lm -pthread

%.o: %.c include/mandelbrot.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

plot: run
	cd src && gnuplot mandel.gp

.PHONY: all clean run plot
