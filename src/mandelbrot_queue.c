#include "../include/mandelbrot_queue.h"

#include <stdlib.h>

int mandelbrot_tile_queue_init(
        MandelbrotTileQueue *queue,
        int capacity
        ) {
    if (!queue || capacity <= 0) {
        return 1;
    }

    queue->tiles = malloc(sizeof(MandelbrotTile) * (size_t)capacity);
    
    if (!queue->tiles) {
        return 1;
    }

    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->closed = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->tiles);
        queue->tiles = NULL;
        return 1;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->tiles);
        queue->tiles = NULL;
        return 1;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->tiles);
        queue->tiles = NULL;
        return 1;
    }

    return 0;
}

void mandelbrot_tile_queue_destroy(
        MandelbrotTileQueue *queue
        ) {
    if (!queue) {
        return;
    }

    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);

    free(queue->tiles);

    queue->tiles = NULL;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->closed = 1;
}

int mandelbrot_tile_queue_push(
        MandelbrotTileQueue *queue,
        const MandelbrotTile *tile
        ) {
    if (!queue || !tile) {
        return 1;
    }

    pthread_mutex_lock(&queue->mutex);

    while (queue->count == queue->capacity &&
           !queue->closed) {

        pthread_cond_wait(
            &queue->not_full,
            &queue->mutex
        );
    }

    if (queue->closed) {
        pthread_mutex_unlock(&queue->mutex);
        return 1;
    }

    queue->tiles[queue->tail] = *tile;

    queue->tail = (queue->tail + 1) % queue->capacity;

    queue->count++;

    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int mandelbrot_tile_queue_pop(
    MandelbrotTileQueue *queue,
    MandelbrotTile *tile
        ) {
    if (!queue || !tile) {
        return 1;
    }

    pthread_mutex_lock(&queue->mutex);

     while (queue->count == 0 &&
           !queue->closed) {

        pthread_cond_wait(
            &queue->not_empty,
            &queue->mutex
        );
    }

    if (queue->count == 0 &&
        queue->closed) {

        pthread_mutex_unlock(&queue->mutex);
        return 1;
    }

    *tile = queue->tiles[queue->head];

    queue->head = (queue->head + 1) % queue->capacity;

    queue->count--;

    pthread_cond_signal(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int mandelbrot_tile_queue_try_pop(
    MandelbrotTileQueue *queue,
    MandelbrotTile *tile
) {
    if (!queue || !tile) {
        return 0;
    }

    pthread_mutex_lock(
        &queue->mutex
    );

    if (queue->count == 0) {
        pthread_mutex_unlock(
            &queue->mutex
        );

        return 0;
    }

    *tile =
        queue->tiles[queue->head];

    queue->head =
        (queue->head + 1) %
        queue->capacity;

    queue->count--;

    pthread_cond_signal(
        &queue->not_full
    );

    pthread_mutex_unlock(
        &queue->mutex
    );

    return 1;
}

void mandelbrot_tile_queue_close(
        MandelbrotTileQueue *queue
        ) {
    if (!queue) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    queue->closed = 1;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);
}
