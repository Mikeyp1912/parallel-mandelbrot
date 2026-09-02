CC = gcc

CFLAGS = -Wall -Wextra -O2 -Iinclude -pthread
LDLIBS = -lm -pthread -lpng

TARGET = mandelbrot

SRC = src/main.c \
      src/mandelbrot_core.c \
      src/mandelbrot_colour.c \
      src/mandelbrot_output.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDLIBS)

%.o: %.c include/mandelbrot.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

clean-render:
	rm -f plot/mandel.png

clean-all: clean clean-render

.PHONY: all run clean clean-render clean-all
