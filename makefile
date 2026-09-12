CC = gcc

CFLAGS = -Wall -Wextra -O2 -Iinclude -pthread
LDLIBS = -lm -pthread -lpng

TARGET = mandelbrot
VIEWER = mandelbrot_viewer

SDL_CFLAGS = $(shell pkg-config --cflags sdl3)
SDL_LIBS = $(shell pkg-config --libs sdl3)


TEST_AVX2 = test_avx2
TEST_TILES = test_tiles
TEST_PROGRESSIVE = test_progressive
TEST_QUEUE = test_queue
TEST_PROGRESSIVE_QUEUE = test_progressive_queue


SRC = src/main.c \
      src/mandelbrot_core.c \
      src/mandelbrot_colour.c \
      src/mandelbrot_output.c \
      src/mandelbrot_avx2.c \
      src/mandelbrot_queue.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDLIBS)

%.o: %.c include/mandelbrot.h
	$(CC) $(CFLAGS) -c $< -o $@

src/mandelbrot_avx2.o: src/mandelbrot_avx2.c include/mandelbrot.h
	$(CC) $(CFLAGS) -mavx2 -c $< -o $@

src/mandelbrot_queue.o: src/mandelbrot_queue.c \
                        include/mandelbrot_queue.h \
                        include/mandelbrot.h
	$(CC) $(CFLAGS) -c $< -o $@



$(VIEWER): src/viewer.c \
           src/mandelbrot_core.o \
           src/mandelbrot_colour.o \
           src/mandelbrot_avx2.o \
           src/mandelbrot_queue.o
	$(CC) $(CFLAGS) $(SDL_CFLAGS) \
	    src/viewer.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_colour.o \
	    src/mandelbrot_avx2.o \
	    src/mandelbrot_queue.o \
	    -o $(VIEWER) \
	    $(LDLIBS) \
	    $(SDL_LIBS)



$(TEST_AVX2): tests/test_avx2.c \
              src/mandelbrot_core.o \
              src/mandelbrot_avx2.o
	$(CC) $(CFLAGS) \
	    tests/test_avx2.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_avx2.o \
	    -o $(TEST_AVX2) \
	    $(LDLIBS)

$(TEST_TILES): tests/test_tiles.c \
               src/mandelbrot_core.o \
               src/mandelbrot_avx2.o
	$(CC) $(CFLAGS) \
	    tests/test_tiles.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_avx2.o \
	    -o $(TEST_TILES) \
	    $(LDLIBS)

$(TEST_PROGRESSIVE): tests/test_progressive.c \
                     src/mandelbrot_core.o \
                     src/mandelbrot_avx2.o
	$(CC) $(CFLAGS) \
	    tests/test_progressive.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_avx2.o \
	    -o $(TEST_PROGRESSIVE) \
	    $(LDLIBS)

$(TEST_QUEUE): tests/test_queue.c \
               src/mandelbrot_queue.o
	$(CC) $(CFLAGS) \
	    tests/test_queue.c \
	    src/mandelbrot_queue.o \
	    -o $(TEST_QUEUE) \
	    $(LDLIBS)

$(TEST_PROGRESSIVE_QUEUE): tests/test_progressive_queue.c \
                           src/mandelbrot_core.o \
                           src/mandelbrot_avx2.o \
                           src/mandelbrot_queue.o
	$(CC) $(CFLAGS) \
	    tests/test_progressive_queue.c \
	    src/mandelbrot_core.o \
	    src/mandelbrot_avx2.o \
	    src/mandelbrot_queue.o \
	    -o $(TEST_PROGRESSIVE_QUEUE) \
	    $(LDLIBS)

test-progressive-queue: $(TEST_PROGRESSIVE_QUEUE)
	./$(TEST_PROGRESSIVE_QUEUE)

test-queue: $(TEST_QUEUE)
	./$(TEST_QUEUE)

test-progressive: $(TEST_PROGRESSIVE)
	./$(TEST_PROGRESSIVE)

test-tiles: $(TEST_TILES)
	./$(TEST_TILES)

test-avx2: $(TEST_AVX2)
	./$(TEST_AVX2)

viewer: $(VIEWER)
	./$(VIEWER)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) \
	      $(TARGET) \
	      $(VIEWER) \
	      $(TEST_AVX2) \
	      $(TEST_TILES) \
	      $(TEST_PROGRESSIVE) \
	      $(TEST_QUEUE) \
	      $(TEST_PROGRESSIVE_QUEUE)

clean-render:
	rm -f plot/mandel.png

clean-all: clean clean-render

.PHONY: all run viewer clean clean-render clean-all \
        test-avx2 test-tiles test-progressive \
        test-queue test-progressive-queue
