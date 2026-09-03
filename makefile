CC = gcc

CFLAGS = -Wall -Wextra -O2 -Iinclude -pthread
LDLIBS = -lm -pthread -lpng

TARGET = mandelbrot
TEST_AVX2 = test_avx2

SRC = src/main.c \
      src/mandelbrot_core.c \
      src/mandelbrot_colour.c \
      src/mandelbrot_output.c \
      src/mandelbrot_avx2.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDLIBS)

%.o: %.c include/mandelbrot.h
	$(CC) $(CFLAGS) -c $< -o $@

src/mandelbrot_avx2.o: src/mandelbrot_avx2.c include/mandelbrot.h
	$(CC) $(CFLAGS) -mavx2 -c $< -o $@

$(TEST_AVX2): tests/test_avx2.c \
              src/mandelbrot_core.o \
              src/mandelbrot_avx2.o
	$(CC) $(CFLAGS) \
	    tests/test_avx2.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_avx2.o \
	    -o $(TEST_AVX2) \
	    $(LDLIBS)

test-avx2: $(TEST_AVX2)
	./$(TEST_AVX2)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_AVX2)

clean-render:
	rm -f plot/mandel.png

clean-all: clean clean-render

.PHONY: all run test-avx2 clean clean-render clean-all
