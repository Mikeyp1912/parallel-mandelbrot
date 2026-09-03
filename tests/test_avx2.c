#include "../include/mandelbrot.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    MandelbrotConfig cfg;

    mandelbrot_set_defaults(&cfg);

    /*
     * Use a moderately sized image so the test is fast,
     * but still covers plenty of different points.
     */
    cfg.width = 800;
    cfg.height = 600;
    cfg.max_iter = 5000;

    /*
     * Seahorse Valley is a good validation region because
     * it contains lots of complicated boundary behaviour.
     */
    cfg.center_x = -0.743643887;
    cfg.center_y = 0.131825904;
    cfg.zoom = 100.0;

    /*
     * Reproduce the same center/zoom -> bounds conversion
     * used by the normal program.
     *
     * If you already have this logic in a reusable function,
     * call that instead.
     */
    double aspect =
        (double)cfg.width /
        (double)cfg.height;

    double view_width =
        3.0 / cfg.zoom;

    double view_height =
        view_width / aspect;

    cfg.x_min =
        cfg.center_x - view_width / 2.0;

    cfg.x_max =
        cfg.center_x + view_width / 2.0;

    cfg.y_min =
        cfg.center_y - view_height / 2.0;

    cfg.y_max =
        cfg.center_y + view_height / 2.0;

    MandelbrotImage scalar_img;
    MandelbrotImage avx2_img;

    if (mandelbrot_image_init(&scalar_img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate scalar image\n");
        return 1;
    }

    if (mandelbrot_image_init(&avx2_img, &cfg) != 0) {
        fprintf(stderr,
                "Failed to allocate AVX2 image\n");

        mandelbrot_image_free(&scalar_img);
        return 1;
    }

    mandelbrot_compute_serial(
        &cfg,
        &scalar_img
    );

    mandelbrot_compute_avx2(
        &cfg,
        &avx2_img
    );

    size_t total_pixels =
        (size_t)cfg.width *
        (size_t)cfg.height;

    size_t iteration_mismatches = 0;
    size_t smooth_mismatches = 0;

    double max_smooth_difference = 0.0;

    for (size_t i = 0; i < total_pixels; i++) {

        if (scalar_img.iterations[i] !=
            avx2_img.iterations[i]) {

            iteration_mismatches++;

            /*
             * Print only the first few mismatches so the
             * terminal doesn't get flooded.
             */
            if (iteration_mismatches <= 10) {
                printf(
                    "Iteration mismatch at pixel %zu: "
                    "scalar=%d avx2=%d\n",
                    i,
                    scalar_img.iterations[i],
                    avx2_img.iterations[i]
                );
            }
        }

        double difference =
            fabs(
                scalar_img.smooth_values[i] -
                avx2_img.smooth_values[i]
            );

        if (difference > max_smooth_difference) {
            max_smooth_difference = difference;
        }

        /*
         * Smooth values involve floating-point maths,
         * so use a very small tolerance rather than
         * requiring bit-for-bit equality.
         */
        if (difference > 1e-12) {
            smooth_mismatches++;

            if (smooth_mismatches <= 10) {
                printf(
                    "Smooth mismatch at pixel %zu: "
                    "scalar=%.15f avx2=%.15f diff=%.15e\n",
                    i,
                    scalar_img.smooth_values[i],
                    avx2_img.smooth_values[i],
                    difference
                );
            }
        }
    }

    printf("\n");
    printf("AVX2 validation results\n");
    printf("-----------------------\n");
    printf("Pixels tested:          %zu\n",
           total_pixels);

    printf("Iteration mismatches:   %zu\n",
           iteration_mismatches);

    printf("Smooth mismatches:      %zu\n",
           smooth_mismatches);

    printf("Max smooth difference:  %.15e\n",
           max_smooth_difference);

    int passed =
        iteration_mismatches == 0 &&
        smooth_mismatches == 0;

    printf("Result:                 %s\n",
           passed ? "PASS" : "FAIL");

    mandelbrot_image_free(&scalar_img);
    mandelbrot_image_free(&avx2_img);

    return passed ? 0 : 1;
}
