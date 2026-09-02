// mandelbrot_colour.c
#include "../include/mandelbrot.h"
#include <stdlib.h>

void mandelbrot_apply_histogram_colouring(const MandelbrotConfig *cfg, MandelbrotImage *img) {
	int max_iter = cfg->max_iter;

	int total = 0;
	for (int i = 0; i < max_iter; i++) {
		total += img->histogram[i];
	}

	if (total == 0){	total = 1;	}
	
	double *cdf = malloc(sizeof(double) * max_iter);

	int running_total = 0;
	for (int i = 0; i < max_iter; i++) {
		running_total += img->histogram[i];
		cdf[i] = (double)running_total / total;
	}

	int total_pixels =cfg->width * cfg->height;

	for (int i = 0; i < total_pixels; i++) {
		int iter = img->iterations[i];

		if (iter == max_iter) {
			img->pixels[i] = 0.0;
		}
		else {
			img->pixels[i] = cdf[iter];
		}
	}

	free(cdf);
}
