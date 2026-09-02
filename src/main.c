// main.c
#include "../include/mandelbrot.h"
#include <stdio.h>

int main(void) {
	MandelbrotConfig cfg;
	MandelbrotImage img;

	mandelbrot_set_defaults(&cfg);

	if (mandelbrot_image_init(&img, &cfg) != 0) {
		printf("Failed to allocate memory\n");
		return 1;
	}

	mandelbrot_compute_serial(&cfg, &img);
	mandelbrot_apply_histogram_colouring(&cfg, &img);

	if (mandelbrot_write_data("mandel.dat", &cfg, &img) != 0) {
		printf("Failed to write output file\n");
		mandelbrot_image_free(&img);
		return 1;
	}

	mandelbrot_image_free(&img);

	printf("Render complete: mandel.dat\n");
	return 0;
}
