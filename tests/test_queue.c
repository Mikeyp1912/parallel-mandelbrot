#include "../include/mandelbrot_queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define TILE_COUNT 1000
#define QUEUE_CAPACITY 16

typedef struct {
    MandelbrotTileQueue *queue;
} ThreadArgs;

static void *producer_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = 0; i < TILE_COUNT; i++) {
        MandelbrotTile tile = {
            .x_start = i,
            .y_start = i * 2,
            .width = 32,
            .height = 32
        };

        if (mandelbrot_tile_queue_push(
                args->queue,
                &tile) != 0) {

            return (void *)1;
        }
    }

    return NULL;
}

static void *consumer_thread(void *arg) {
    ThreadArgs *args =
        (ThreadArgs *)arg;

    for (int i = 0; i < TILE_COUNT; i++) {
        MandelbrotTile tile;

        if (mandelbrot_tile_queue_pop(
                args->queue,
                &tile) != 0) {

            return (void *)1;
        }

        /*
         * Because there is one producer and one consumer,
         * FIFO ordering should be preserved exactly.
         */
        if (tile.x_start != i ||
            tile.y_start != i * 2 ||
            tile.width != 32 ||
            tile.height != 32) {

            fprintf(stderr,
                    "Tile mismatch at index %d\n",
                    i);

            return (void *)1;
        }
    }

    return NULL;
}

int main(void) {
    MandelbrotTileQueue queue;

    if (mandelbrot_tile_queue_init(
            &queue,
            QUEUE_CAPACITY) != 0) {

        fprintf(stderr,
                "Failed to initialise queue\n");

        return 1;
    }

    ThreadArgs args = {
        .queue = &queue
    };

    pthread_t producer;
    pthread_t consumer;

    int producer_result =
        pthread_create(
            &producer,
            NULL,
            producer_thread,
            &args
        );

    if (producer_result != 0) {
        fprintf(stderr,
                "Failed to create producer thread\n");

        mandelbrot_tile_queue_destroy(&queue);

        return 1;
    }

    int consumer_result =
        pthread_create(
            &consumer,
            NULL,
            consumer_thread,
            &args
        );

    if (consumer_result != 0) {
        fprintf(stderr,
                "Failed to create consumer thread\n");

        pthread_cancel(producer);
        pthread_join(producer, NULL);

        mandelbrot_tile_queue_destroy(&queue);

        return 1;
    }

    void *producer_status = NULL;
    void *consumer_status = NULL;

    pthread_join(
        producer,
        &producer_status
    );

    pthread_join(
        consumer,
        &consumer_status
    );

    int success =
        producer_status == NULL &&
        consumer_status == NULL;

    printf("\nTile queue validation\n");
    printf("----------------------------------------\n");
    printf("Tiles transferred:      %d\n", TILE_COUNT);
    printf("Queue capacity:         %d\n", QUEUE_CAPACITY);
    printf("Producer result:        %s\n",
           producer_status == NULL ? "PASS" : "FAIL");
    printf("Consumer result:        %s\n",
           consumer_status == NULL ? "PASS" : "FAIL");
    printf("Overall result:         %s\n",
           success ? "PASS" : "FAIL");

    mandelbrot_tile_queue_destroy(&queue);

    return success ? 0 : 1;
}
