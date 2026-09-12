#include "../include/mandelbrot.h"
#include "../include/mandelbrot_queue.h"

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    MandelbrotConfig *cfg;
    MandelbrotImage *img;
    MandelbrotTileQueue *queue;
    int result;
    int finished;
    pthread_mutex_t finished_mutex;
} ViewerRenderState;

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

static void *viewer_render_thread(void *arg) {
    ViewerRenderState *state =
        (ViewerRenderState *)arg;

    state->result =
        mandelbrot_compute_pthreads_progressive(
            state->cfg,
            state->img,
            queue_tile_callback,
            state->queue
        );

    pthread_mutex_lock(
        &state->finished_mutex
    );

    state->finished = 1;

    pthread_mutex_unlock(
        &state->finished_mutex
    );

    return NULL;
}

static void update_tile_preview(
    SDL_Texture *texture,
    const MandelbrotConfig *cfg,
    const MandelbrotImage *img,
    const MandelbrotTile *tile,
    uint8_t *pixels
) {
    int x_end =
        tile->x_start + tile->width;

    int y_end =
        tile->y_start + tile->height;

    if (x_end > cfg->width) {
        x_end = cfg->width;
    }

    if (y_end > cfg->height) {
        y_end = cfg->height;
    }

    for (int y = tile->y_start; y < y_end; y++) {
        for (int x = tile->x_start; x < x_end; x++) {
            size_t index =
                (size_t)y *
                (size_t)cfg->width +
                (size_t)x;

            uint8_t r;
            uint8_t g;
            uint8_t b;

            mandelbrot_colour_pixel_preview(
                cfg,
                img,
                index,
                &r,
                &g,
                &b
            );

            size_t pixel =
                index * 4;

            pixels[pixel]     = r;
            pixels[pixel + 1] = g;
            pixels[pixel + 2] = b;
            pixels[pixel + 3] = 255;
        }
    }

    SDL_Rect rect = {
        .x = tile->x_start,
        .y = tile->y_start,
        .w = x_end - tile->x_start,
        .h = y_end - tile->y_start
    };

    const uint8_t *tile_pixels =
        pixels +
        (
            (size_t)tile->y_start *
            (size_t)cfg->width +
            (size_t)tile->x_start
        ) * 4;

    SDL_UpdateTexture(
        texture,
        &rect,
        tile_pixels,
        cfg->width * 4
    );
}

int main(void) {
    MandelbrotConfig cfg;
    MandelbrotImage img;

    mandelbrot_set_defaults(&cfg);

    cfg.width = 1280;
    cfg.height = 720;
    cfg.max_iter = 5000;

    cfg.center_x = -0.743643887;
    cfg.center_y = 0.131825904;
    cfg.zoom = 100.0;

    cfg.threads = 16;
    cfg.tile_size = 32;
    cfg.backend = MANDELBROT_BACKEND_PTHREAD_AVX2;

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
                "Failed to allocate Mandelbrot image\n");
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr,
                "SDL_Init failed: %s\n",
                SDL_GetError());

        mandelbrot_image_free(&img);
        return 1;
    }

    SDL_Window *window =
        SDL_CreateWindow(
            "Mandelbrot Viewer",
            1280,
            720,
            SDL_WINDOW_RESIZABLE
        );

    if (!window) {
        fprintf(stderr,
                "SDL_CreateWindow failed: %s\n",
                SDL_GetError());

        SDL_Quit();
        mandelbrot_image_free(&img);
        return 1;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(
            window,
            NULL
        );

    if (!renderer) {
        fprintf(stderr,
                "SDL_CreateRenderer failed: %s\n",
                SDL_GetError());

        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);
        return 1;
    }

    SDL_Texture *texture =
        SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            cfg.width,
            cfg.height
        );

    if (!texture) {
        fprintf(stderr,
                "SDL_CreateTexture failed: %s\n",
                SDL_GetError());

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);
        return 1;
    }

    size_t pixel_count =
        (size_t)cfg.width *
        (size_t)cfg.height;

    uint8_t *pixels =
        calloc(
            pixel_count,
            4
        );

    if (!pixels) {
        fprintf(stderr,
                "Failed to allocate viewer pixel buffer\n");

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);
        return 1;
    }

    for (size_t i = 0; i < pixel_count; i++) {
        size_t pixel = i * 4;

        pixels[pixel]     = 0;
        pixels[pixel + 1] = 0;
        pixels[pixel + 2] = 0;
        pixels[pixel + 3] = 255;
    }

    SDL_UpdateTexture(
        texture,
        NULL,
        pixels,
        cfg.width * 4
    );

    MandelbrotTileQueue queue;

    if (mandelbrot_tile_queue_init(
            &queue,
            128) != 0) {

        fprintf(stderr,
                "Failed to initialise tile queue\n");

        free(pixels);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);

        return 1;
    }

    ViewerRenderState render_state = {
        .cfg = &cfg,
        .img = &img,
        .queue = &queue,
        .result = 1,
        .finished = 0
    };

    if (pthread_mutex_init(
            &render_state.finished_mutex,
            NULL) != 0) {

        fprintf(stderr,
                "Failed to initialise render state mutex\n");

        mandelbrot_tile_queue_destroy(&queue);

        free(pixels);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);

        return 1;
    }

    pthread_t render_thread;

    if (pthread_create(
            &render_thread,
            NULL,
            viewer_render_thread,
            &render_state) != 0) {

        fprintf(stderr,
                "Failed to create render thread\n");

        pthread_mutex_destroy(
            &render_state.finished_mutex
        );

        mandelbrot_tile_queue_destroy(&queue);

        free(pixels);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        mandelbrot_image_free(&img);

        return 1;
    }

    int running = 1;
    int final_colour_applied = 0;

    int tiles_x =
        (cfg.width + cfg.tile_size - 1) /
        cfg.tile_size;

    int tiles_y =
        (cfg.height + cfg.tile_size - 1) /
        cfg.tile_size;

    int total_tiles =
        tiles_x * tiles_y;

    int consumed_tiles = 0;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        MandelbrotTile tile;

        int tiles_this_frame = 0;
        const int max_tiles_per_frame = 4;

        while (tiles_this_frame < max_tiles_per_frame &&
               mandelbrot_tile_queue_try_pop(&queue, &tile)) {

            update_tile_preview(
                texture,
                &cfg,
                &img,
                &tile,
                pixels
            );

            tiles_this_frame++;
            consumed_tiles++;
        }


        pthread_mutex_lock(
            &render_state.finished_mutex
        );

        int finished =
            render_state.finished;

        pthread_mutex_unlock(
            &render_state.finished_mutex
        );

        if (finished &&
            consumed_tiles == total_tiles &&
            !final_colour_applied &&
            render_state.result == 0) {

            printf(
                "Applying final colours after %d/%d tiles\n",
                consumed_tiles,
                total_tiles
            );

            mandelbrot_build_colour_cdf(
                &cfg,
                &img
            );

            for (size_t i = 0;
                 i < pixel_count;
                 i++) {

                uint8_t r;
                uint8_t g;
                uint8_t b;

                mandelbrot_colour_pixel(
                    &cfg,
                    &img,
                    i,
                    &r,
                    &g,
                    &b
                );

                size_t pixel =
                    i * 4;

                pixels[pixel]     = r;
                pixels[pixel + 1] = g;
                pixels[pixel + 2] = b;
                pixels[pixel + 3] = 255;
            }

            SDL_UpdateTexture(
                texture,
                NULL,
                pixels,
                cfg.width * 4
            );

            final_colour_applied = 1;
        }

        SDL_SetRenderDrawColor(
            renderer,
            0,
            0,
            0,
            255
        );

        SDL_RenderClear(renderer);

        SDL_RenderTexture(
            renderer,
            texture,
            NULL,
            NULL
        );

        SDL_RenderPresent(renderer);

        SDL_Delay(1);
    }

    pthread_join(render_thread, NULL);

    pthread_mutex_destroy(
        &render_state.finished_mutex
    );

    mandelbrot_tile_queue_destroy(
        &queue
    );

    free(pixels);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    mandelbrot_image_free(&img);

    return 0;
}
