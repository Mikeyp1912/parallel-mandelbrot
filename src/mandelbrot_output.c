// mandelbrot_output.c
#include "../include/mandelbrot.h"
#include <stdio.h>

int mandelbrot_write_data(const char *filename,
						  const MandelbrotConfig *cfg,
						  const MandelbrotImage *img) {
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {	return 1;	}

	for (int y = 0; y < cfg->height; y++) {
		for (int x = 0; x < cfg->width; x++) {
			int index = y * cfg->width + x;

			fprintf(fp, "%d %d %.12f\n", x, y, img->pixels[index]);
		}

		fprintf(fp, "\n");
	}					

	fclose(fp);
	return 0;
 }
