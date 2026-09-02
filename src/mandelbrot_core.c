// mandelbrot_core.c
#include "../include/mandelbrot.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

void mandelbrot_set_defaults(MandelbrotConfig *cfg) {
	cfg->width = 1000;
	cfg->height = 1000;
	cfg->max_iter = 1000;

	cfg->x_min = -2.0;
	cfg->x_max = 1.0;
	cfg->y_min = -1.5;
	cfg->y_max = 1.5;

    cfg->gamma = 1.5;
}

void mandelbrot_print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("\n");
    printf("Options:\n");
    printf("  --width <pixels>      Image width\n");
    printf("  --height <pixels>     Image height\n");
    printf("  --iterations <count>  Maximum iterations\n");
    printf("  --gamma <value>       Colour gamma correction\n");
    printf("  --xmin <value>        Minimum real coordinate\n");
    printf("  --xmax <value>        Maximum real coordinate\n");
    printf("  --ymin <value>        Minimum imaginary coordinate\n");
    printf("  --ymax <value>        Maximum imaginary coordinate\n");
}

int mandelbrot_parse_args(MandelbrotConfig *cfg, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->height = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--iterations") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->max_iter = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--gamma") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->gamma = atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--xmin") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->x_min = atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--xmax") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->x_max = atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--ymin") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->y_min = atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--ymax") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->y_max = atof(argv[++i]);
        }

        else {
            return 1;
        }
    }

    if (cfg->width <= 0) {
        fprintf(stderr, "Error: width must be greater than 0\n");
        return 1;
    }

    if (cfg->height <= 0) {
        fprintf(stderr, "Error: height must be greater than 0\n");
        return 1;
    }

    if (cfg->max_iter <= 0) {
        fprintf(stderr, "Error: iterations must be greater than 0\n");
        return 1;
    }

    if (cfg->gamma <= 0.0) {
        fprintf(stderr, "Error: gamma must be greater than 0\n");
        return 1;
    }
    
    if (cfg->x_min >= cfg->x_max) {
        fprintf(stderr, "Error: xmin must be less than xmax\n");
        return 1;
    }

    if (cfg->y_min >= cfg->y_max) {
        fprintf(stderr, "Error: ymin must be less than ymax\n");
        return 1;
    }

    return 0;

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


MandelbrotPointResult mandelbrot_iterations(double cr, double ci, int max_iter) {
	double zr = 0.0;
	double zi = 0.0;

	int iter = 0;

	while (zr * zr + zi * zi <= 4.0 && iter < max_iter) {
		double temp = zr * zr - zi * zi + cr;
		zi = 2.0 * zr * zi + ci;
		zr = temp;

		iter++;
	}

	MandelbrotPointResult result;

    result.iterations = iter;

    if (iter == max_iter) {
        result.smooth_value = 0.0;
    }
    else {
        double magnitude = sqrt(zr * zr + zi * zi);

        result.smooth_value = iter + 1.0 - log(log(magnitude)) / log(2.0);
    }

    return result;
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

			MandelbrotPointResult result = mandelbrot_iterations(cr, ci, cfg->max_iter);

			int index = y * width + x;

			img->iterations[index] = result.iterations;
            img->smooth_values[index] = result.smooth_value;

			if (result.iterations < cfg->max_iter) {
				img->histogram[result.iterations]++;
			}
		}
	}
	
}
