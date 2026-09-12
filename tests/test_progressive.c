#include "../include/mandelbrot.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int count;
    pthread_mutex_t mutex;
} CallbackCounter;

static void tile_complete_callback(
    const MandelbrotTile *tile,
    void *user_data
) {
    (void)tile;

    CallbackCounter *counter =
        (CallbackCounter *)user_data;

    pthread_mutex_lock(&counter->mutex);

    counter->count++;

    pthread_mutex_unlock(&counter->mutex);
}

int main(void) {
    MandelbrotConfig cfg;
    MandelbrotImage img;

    mandelbrot_set_defaults(&cfg);

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
     * Convert centre/zoom to explicit bounds.
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

    CallbackCounter counter = {
        .count = 0
    };

    if (pthread_mutex_init(
            &counter.mutex,
            NULL) != 0) {

        fprintf(stderr,
                "Failed to initialise callback mutex\n");

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

    int result =
        mandelbrot_compute_pthreads_progressive(
            &cfg,
            &img,
            tile_complete_callback,
            &counter
        );

    printf("\nProgressive callback validation\n");
    printf("----------------------------------------\n");

    printf("Tiles expected:         %d\n",
           expected_tiles);

    printf("Callbacks received:     %d\n",
           counter.count);

    if (result != 0) {
        printf("Renderer result:        FAIL\n");
    }
    else {
        printf("Renderer result:        PASS\n");
    }

    int success =
        result == 0 &&
        counter.count == expected_tiles;

    printf("Overall result:         %s\n",
           success ? "PASS" : "FAIL");

    pthread_mutex_destroy(&counter.mutex);

    mandelbrot_image_free(&img);

    return success ? 0 : 1;
}
