// mandelbrot_core.c
#include "../include/mandelbrot.h"
#include <stdlib.h>
#include <stdio.h>
// #include <math.h>

void mandelbrot_set_defaults(MandelbrotConfig *cfg) {
	cfg->width = 1000;
	cfg->height = 1000;
	cfg->max_iter = 1000;

	cfg->x_min = -2.0;
	cfg->x_max = 1.0;
	cfg->y_min = -1.5;
	cfg->y_max = 1.5;
}


int mandelbrot_image_init(MandelbrotImage *img, const MandelbrotConfig *cfg) {
	int total_pixels = cfg->width * cfg->height;

	img->iterations     = malloc(sizeof(int) * total_pixels);
	img->histogram      = calloc(cfg->max_iter, sizeof(int));
    img->smooth_values  = malloc(sizeof(double) * total_pixels);
	img->pixels		    = malloc(sizeof(double) * total_pixels);

	if (!img->iterations ||
        !img->histogram ||
        !img->smooth_values ||
        !img->pixels) {
		return 1;
	} 

	return 0;
}


void mandelbrot_image_free(MandelbrotImage *img) {
	free(img->iterations);
	free(img->histogram);
    free(img->smooth_values);
	free(img->pixels);

	img->iterations = NULL;
	img->histogram = NULL;
    img->smooth_values = NULL;
	img->pixels = NULL;
}


int mandelbrot_iterations(double cr, double ci, int max_iter) {
	double zr = 0.0;
	double zi = 0.0;

	int iter = 0;

	while (zr*zr + zi*zi <= 4.0 && iter < max_iter) {
		double temp = zr*zr - zi*zi + cr;
		zi = 2.0 * zr * zi + ci;
		zr = temp;

		iter++;
	}

	return iter;
}


void mandelbrot_compute_serial(const MandelbrotConfig * cfg, MandelbrotImage *img) {
	int width = cfg->width;
	int height = cfg->height;

	double x_scale = (cfg->x_max - cfg->x_min) / width;
	double y_scale = (cfg->y_max - cfg->y_min) / height;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double cr = cfg->x_min + x * x_scale;
			double ci = cfg->y_min + y * y_scale;

			int iter = mandelbrot_iterations(cr, ci, cfg->max_iter);

			int index = y * width + x;
			img->iterations[index] = iter;

			if (iter < cfg->max_iter) {
				img->histogram[iter]++;
			}
		}
	}
	
}
