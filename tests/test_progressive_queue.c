#include "../include/mandelbrot.h"
#include "../include/mandelbrot_queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 32

typedef struct {
    MandelbrotConfig *cfg;
    MandelbrotImage *img;
    MandelbrotTileQueue *queue;
    int result;
} RenderThreadArgs;

static void queue_tile_callback(
    const MandelbrotTile *tile,
    void *user_data
) {
    MandelbrotTileQueue *queue =
        (MandelbrotTileQueue *)user_data;

    mandelbrot_tile_queue_push(
        queue,
        tile
    );
}

static void *render_thread(void *arg) {
    RenderThreadArgs *args =
        (RenderThreadArgs *)arg;

    args->result =
        mandelbrot_compute_pthreads_progressive(
            args->cfg,
            args->img,
            queue_tile_callback,
            args->queue
        );

    return NULL;
}

int main(void) {
    MandelbrotConfig cfg;
    MandelbrotImage img;
    MandelbrotTileQueue queue;

    mandelbrot_set_defaults(&cfg);

    /*
     * Awkward dimensions deliberately chosen to exercise
     * partial edge tiles.
     */
    cfg.width = 803;
    cfg.height = 607;
    cfg.max_iter = 1000;

    cfg.center_x = -0.743643887;
    cfg.center_y = 0.131825904;
    cfg.zoom = 100.0;

    cfg.tile_size = 32;
    cfg.threads = 8;

    cfg.backend =
        MANDELBROT_BACKEND_PTHREAD_AVX2;

    /*
     * Convert centre/zoom into explicit bounds.
     */
    double x_span = 3.0 / cfg.zoom;

    double aspect =
        (double)cfg.height /
        (double)cfg.width;

    double y_span =
        x_span * aspect;

    cfg.x_min =
        cfg.center_x - x_span / 2.0;

    cfg.x_max =
        cfg.center_x + x_span / 2.0;

    cfg.y_min =
        cfg.center_y - y_span / 2.0;

    cfg.y_max =
        cfg.center_y + y_span / 2.0;

    if (mandelbrot_image_init(&img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate image\n");
        return 1;
    }

    if (mandelbrot_tile_queue_init(
            &queue,
            QUEUE_CAPACITY) != 0) {

        fprintf(stderr,
                "Failed to initialise tile queue\n");

        mandelbrot_image_free(&img);
        return 1;
    }

    int tiles_x =
        (cfg.width + cfg.tile_size - 1) /
        cfg.tile_size;

    int tiles_y =
        (cfg.height + cfg.tile_size - 1) /
        cfg.tile_size;

    int expected_tiles =
        tiles_x * tiles_y;

    /*
     * Track which tile positions have already been received.
     */
    int *seen =
        calloc(
            (size_t)expected_tiles,
            sizeof(int)
        );

    if (!seen) {
        fprintf(stderr,
                "Failed to allocate tile tracking array\n");

        mandelbrot_tile_queue_destroy(&queue);
        mandelbrot_image_free(&img);

        return 1;
    }

    RenderThreadArgs render_args = {
        .cfg = &cfg,
        .img = &img,
        .queue = &queue,
        .result = 1
    };

    pthread_t renderer;

    if (pthread_create(
            &renderer,
            NULL,
            render_thread,
            &render_args) != 0) {

        fprintf(stderr,
                "Failed to create render thread\n");

        free(seen);
        mandelbrot_tile_queue_destroy(&queue);
        mandelbrot_image_free(&img);

        return 1;
    }

    int duplicate_tiles = 0;
    int invalid_tiles = 0;

    /*
     * Consume exactly the number of tiles expected from
     * the scheduler.
     */
    for (int i = 0; i < expected_tiles; i++) {
        MandelbrotTile tile;

        if (mandelbrot_tile_queue_pop(
                &queue,
                &tile) != 0) {

            fprintf(stderr,
                    "Failed to pop tile from queue\n");

            invalid_tiles++;
            break;
        }

        /*
         * Convert pixel coordinates back into scheduler
         * tile coordinates.
         */
        int tile_x =
            tile.x_start / cfg.tile_size;

        int tile_y =
            tile.y_start / cfg.tile_size;

        if (tile_x < 0 ||
            tile_x >= tiles_x ||
            tile_y < 0 ||
            tile_y >= tiles_y) {

            invalid_tiles++;
            continue;
        }

        int index =
            tile_y * tiles_x + tile_x;

        if (seen[index]) {
            duplicate_tiles++;
        }
        else {
            seen[index] = 1;
        }
    }

    pthread_join(renderer, NULL);

    int missing_tiles = 0;

    for (int i = 0; i < expected_tiles; i++) {
        if (!seen[i]) {
            missing_tiles++;
        }
    }

    int success =
        render_args.result == 0 &&
        duplicate_tiles == 0 &&
        invalid_tiles == 0 &&
        missing_tiles == 0;

    printf("\nProgressive queue validation\n");
    printf("----------------------------------------\n");
    printf("Tiles expected:         %d\n",
           expected_tiles);
    printf("Duplicate tiles:        %d\n",
           duplicate_tiles);
    printf("Invalid tiles:          %d\n",
           invalid_tiles);
    printf("Missing tiles:          %d\n",
           missing_tiles);
    printf("Renderer result:        %s\n",
           render_args.result == 0
               ? "PASS"
               : "FAIL");
    printf("Overall result:         %s\n",
           success ? "PASS" : "FAIL");

    free(seen);

    mandelbrot_tile_queue_destroy(&queue);
    mandelbrot_image_free(&img);

    return success ? 0 : 1;
}
