// mandelbrot_colour.c
#include "../include/mandelbrot.h"
#include <stdlib.h>
#include <math.h>

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
            double smooth = img->smooth_values[i];

            int lower = (int)smooth;
            double fraction = smooth - lower;

            if (lower < 0) {
                lower = 0;
            }

            if (lower >= max_iter - 1) {
                img->pixels[i] = cdf[max_iter - 1];
            }
            else {
                double c1 = cdf[lower];
                double c2 = cdf[lower + 1];

                double colour = c1 + fraction * (c2 - c1);
                img->pixels[i] = pow(colour, cfg->gamma);
            }
        }
	}

	free(cdf);
}
