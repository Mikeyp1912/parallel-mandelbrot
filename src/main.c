// main.c
#include "../include/mandelbrot.h"
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
	MandelbrotConfig cfg;
	MandelbrotImage img;

	mandelbrot_set_defaults(&cfg);

    if (mandelbrot_parse_args(&cfg, argc, argv) != 0) {
        fprintf(stderr,
                "\nTry '%s --help' or '%s -h' for usage information.\n",
                argv[0], argv[0]);
        return 1;
    }

	if (mandelbrot_image_init(&img, &cfg) != 0) {
		printf("Failed to allocate memory\n");
		return 1;
	}
    
    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    if (cfg.threads == 1) {
        mandelbrot_compute_serial(&cfg, &img);
    }
    else {
        if (mandelbrot_compute_pthreads(&cfg, &img) != 0) {
            fprintf(stderr, "Error: parallel Mandelbrot computation failed\n");
            mandelbrot_image_free(&img);
            return 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

	if (!cfg.no_output) {
        if (mandelbrot_build_colour_cdf(&cfg, &img) != 0) {
            fprintf(stderr, "Error: failed to build colour CDF\n");
            mandelbrot_image_free(&img);
            return 1;
        }

        if (mandelbrot_write_png(cfg.output_file,
                                 &cfg,
                                 &img) != 0) {
            fprintf(stderr, "Failed to write PNG output\n");
            mandelbrot_image_free(&img);
            return 1;
        }
    }

	mandelbrot_image_free(&img);

    printf("\n");
    if (cfg.no_output) {
        printf("Compute complete (output disabled)\n");
    }
    else {
        printf("Render complete: %s\n", cfg.output_file);
    }
    printf("Resolution: %dx%d\n", cfg.width, cfg.height);
    printf("Iterations: %d\n", cfg.max_iter);
    printf("Gamma: %.2f\n", cfg.gamma);
    if (cfg.threads == 1) {
        printf("Mode: Serial\n");
    }
    else {
        printf("Mode: Pthreads (%d threads)\n", cfg.threads);
        printf("Chunk size: %d rows\n", cfg.chunk_size);
    }
    printf("Bounds: x=[%.12f, %.12f], y=[%.12f, %.12f]\n",
           cfg.x_min, cfg.x_max,
           cfg.y_min, cfg.y_max);
    printf("Compute time: %.6f seconds\n", elapsed);

	return 0;
}
