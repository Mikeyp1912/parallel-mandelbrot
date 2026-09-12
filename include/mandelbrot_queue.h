#ifndef MANDELBROT_QUEUE_H
#define MANDELBROT_QUEUE_H

#include "mandelbrot.h"

#include <pthread.h>

typedef struct {
    MandelbrotTile *tiles;

    int capacity;
    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} MandelbrotTileQueue;

int mandelbrot_tile_queue_init(
        MandelbrotTileQueue *queue,
        int capacity
        );

void mandelbrot_tile_queue_destroy(
        MandelbrotTileQueue *queue
        );

int mandelbrot_tile_queue_push(
        MandelbrotTileQueue *queue,
        const MandelbrotTile *tile
        );

int mandelbrot_tile_queue_pop(
        MandelbrotTileQueue *queue,
        MandelbrotTile *tile
        );

#endif
