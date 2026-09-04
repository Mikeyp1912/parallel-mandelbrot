#include "../include/mandelbrot.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_images(
    const MandelbrotConfig *cfg,
    const MandelbrotImage *reference,
    const MandelbrotImage *candidate,
    const char *name
) {
    size_t total_pixels =
        (size_t)cfg->width * (size_t)cfg->height;

    size_t iteration_mismatches = 0;
    size_t smooth_mismatches = 0;
    double max_smooth_difference = 0.0;

    for (size_t i = 0; i < total_pixels; i++) {
        if (reference->iterations[i] != candidate->iterations[i]) {
            iteration_mismatches++;
        }

        double difference =
            fabs(reference->smooth_values[i] -
                 candidate->smooth_values[i]);

        if (difference > max_smooth_difference) {
            max_smooth_difference = difference;
        }

        if (difference > 1e-12) {
            smooth_mismatches++;
        }
    }

    size_t histogram_mismatches = 0;

    for (int i = 0; i < cfg->max_iter; i++) {
        if (reference->histogram[i] != candidate->histogram[i]) {
            histogram_mismatches++;
        }
    }

    printf("\n%s\n", name);
    printf("----------------------------------------\n");
    printf("Pixels tested:          %zu\n", total_pixels);
    printf("Iteration mismatches:   %zu\n", iteration_mismatches);
    printf("Smooth mismatches:      %zu\n", smooth_mismatches);
    printf("Histogram mismatches:   %zu\n", histogram_mismatches);
    printf("Max smooth difference:  %.15e\n",
           max_smooth_difference);

    if (iteration_mismatches == 0 &&
        smooth_mismatches == 0 &&
        histogram_mismatches == 0) {

        printf("Result:                 PASS\n");
        return 0;
    }

    printf("Result:                 FAIL\n");
    return 1;
}

int main(void) {
    MandelbrotConfig cfg;

    MandelbrotImage serial_img;
    MandelbrotImage pthread_img;
    MandelbrotImage pthread_avx2_img;

    mandelbrot_set_defaults(&cfg);

    /*
     * Deliberately awkward dimensions:
     *
     * - not divisible by tile size
     * - width not divisible by AVX2 width of 4
     *
     * This exercises edge tiles and AVX2 scalar tails.
     */
    cfg.width = 803;
    cfg.height = 607;
    cfg.max_iter = 5000;

    cfg.center_x = -0.743643887;
    cfg.center_y = 0.131825904;
    cfg.zoom = 100.0;

    cfg.tile_size = 32;
    cfg.threads = 8;

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

    if (mandelbrot_image_init(&serial_img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate serial image\n");
        return 1;
    }

    if (mandelbrot_image_init(&pthread_img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate pthread image\n");

        mandelbrot_image_free(&serial_img);
        return 1;
    }

    if (mandelbrot_image_init(&pthread_avx2_img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate pthread AVX2 image\n");

        mandelbrot_image_free(&pthread_img);
        mandelbrot_image_free(&serial_img);
        return 1;
    }

    /*
     * Reference render.
     */
    cfg.backend =
        MANDELBROT_BACKEND_SERIAL;

    mandelbrot_compute_serial(
        &cfg,
        &serial_img
    );

    /*
     * Tiled scalar pthread render.
     */
    cfg.backend =
        MANDELBROT_BACKEND_PTHREAD;

    if (mandelbrot_compute_pthreads(
            &cfg,
            &pthread_img) != 0) {

        fprintf(stderr,
                "Pthread render failed\n");

        mandelbrot_image_free(&pthread_avx2_img);
        mandelbrot_image_free(&pthread_img);
        mandelbrot_image_free(&serial_img);

        return 1;
    }

    /*
     * Tiled pthread + AVX2 render.
     */
    cfg.backend =
        MANDELBROT_BACKEND_PTHREAD_AVX2;

    if (mandelbrot_compute_pthreads_avx2(
            &cfg,
            &pthread_avx2_img) != 0) {

        fprintf(stderr,
                "Pthread AVX2 render failed\n");

        mandelbrot_image_free(&pthread_avx2_img);
        mandelbrot_image_free(&pthread_img);
        mandelbrot_image_free(&serial_img);

        return 1;
    }

    int failures = 0;

    failures += compare_images(
        &cfg,
        &serial_img,
        &pthread_img,
        "Pthreads tile validation"
    );

    failures += compare_images(
        &cfg,
        &serial_img,
        &pthread_avx2_img,
        "Pthreads + AVX2 tile validation"
    );

    mandelbrot_image_free(&pthread_avx2_img);
    mandelbrot_image_free(&pthread_img);
    mandelbrot_image_free(&serial_img);

    printf("\nOverall result: %s\n",
           failures == 0 ? "PASS" : "FAIL");

    return failures == 0 ? 0 : 1;
}
