// mandelbrot_core.c
#include "../include/mandelbrot.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    int next_row;
    int height;
    int chunk_size;
    pthread_mutex_t mutex;
} MandelbrotScheduler;

typedef struct {
    const MandelbrotConfig *cfg;
    MandelbrotImage *img;
    MandelbrotScheduler *scheduler;
    int *local_histogram;
} MandelbrotThreadArgs;


void mandelbrot_set_defaults(MandelbrotConfig *cfg) {
	cfg->width = 1000;
	cfg->height = 1000;
	cfg->max_iter = 1000;

	cfg->x_min = -2.0;
	cfg->x_max = 1.0;
	cfg->y_min = -1.5;
	cfg->y_max = 1.5;
        
    cfg->center_x = -0.5;
    cfg->center_y = 0.0;
    cfg->zoom = 1.0;

    cfg->gamma = 4.0;
    cfg->no_output = 0;

    cfg->threads = 1;
    cfg->chunk_size = 1;
}

void mandelbrot_print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("\n");
    printf("General Options:\n");
    printf("  --width <pixels>      Image width\n");
    printf("  --height <pixels>     Image height\n");
    printf("  --iterations <count>  Maximum iterations\n");
    printf("  --gamma <value>       Colour gamma correction\n");
    printf("  --no-output           Skip colouring and output file generation\n");
    printf("\nManual Bounds Options:\n");
    printf("  --xmin <value>        Minimum real coordinate\n");
    printf("  --xmax <value>        Maximum real coordinate\n");
    printf("  --ymin <value>        Minimum imaginary coordinate\n");
    printf("  --ymax <value>        Maximum imaginary coordinate\n");
    printf("\nCenter/Zoom Options:\n");
    printf("  --center-x <value>    Centre real coordinate\n");
    printf("  --center-y <value>    Centre imaginary coordinate\n");
    printf("  --zoom <value>        Zoom factor (1.0 = full view)\n");
    printf("\nParallelisation Options:\n");
    printf("  --threads <count>     Number of worker threads\n");
    printf("  --chunk-size <rows>   Rows assigned per scheduler request\n");
}

int mandelbrot_parse_args(MandelbrotConfig *cfg, int argc, char *argv[]) {
    int bounds_mode_used = 0;
    int center_zoom_mode_used = 0;

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
            bounds_mode_used = 1;

        }
        else if (strcmp(argv[i], "--xmax") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->x_max = atof(argv[++i]);
            bounds_mode_used = 1;
        }
        else if (strcmp(argv[i], "--ymin") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->y_min = atof(argv[++i]);
            bounds_mode_used = 1;
        }
        else if (strcmp(argv[i], "--ymax") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->y_max = atof(argv[++i]);
            bounds_mode_used = 1;
        }
        else if (strcmp(argv[i], "--center-x") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->center_x = atof(argv[++i]);
            center_zoom_mode_used = 1;
        }
        else if (strcmp(argv[i], "--center-y") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->center_y = atof(argv[++i]);
            center_zoom_mode_used = 1;
        }
        else if (strcmp(argv[i], "--zoom") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->zoom = atof(argv[++i]);
            center_zoom_mode_used = 1;
        }
        else if (strcmp(argv[i], "--threads") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->threads = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--chunk-size") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }

            cfg->chunk_size = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--no-output") == 0) {
            cfg->no_output = 1;
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

    
    if (cfg->x_min >= cfg->x_max) {
        fprintf(stderr, "Error: xmin must be less than xmax\n");
        return 1;
    }

    if (cfg->y_min >= cfg->y_max) {
        fprintf(stderr, "Error: ymin must be less than ymax\n");
        return 1;
    }

    if (cfg->gamma <= 0.0) {
        fprintf(stderr, "Error: gamma must be greater than 0\n");
        return 1;
    }

    if (bounds_mode_used && center_zoom_mode_used) {
        fprintf(stderr,
                "Error: cannot combine manual bounds with center/zoom options\n");
        return 1;
    }

    if (cfg->zoom <= 0.0) {
        fprintf(stderr, "Error: zoom must be greater than 0\n");
        return 1;
    }
    
    if (center_zoom_mode_used) {
        double x_span = 3.0 / cfg->zoom;

        double aspect =
            (double)cfg->height / (double)cfg->width;

        double y_span = x_span * aspect;

        cfg->x_min = cfg->center_x - x_span / 2.0;
        cfg->x_max = cfg->center_x + x_span / 2.0;

        cfg->y_min = cfg->center_y - y_span / 2.0;
        cfg->y_max = cfg->center_y + y_span / 2.0;
    }
    
    if (cfg->threads <= 0) {
        fprintf(stderr, "Error: threads must be greater than 0\n");
        return 1;
    }

    if (cfg->chunk_size <= 0) {
        fprintf(stderr, "Error: chunk size must be greater than 0\n");
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

static int scheduler_get_next_chunk(MandelbrotScheduler *scheduler, int *start_row, int *end_row) {
    pthread_mutex_lock(&scheduler->mutex);

    if (scheduler->next_row >= scheduler->height) {
        pthread_mutex_unlock(&scheduler->mutex);
        return 0;
    }

    *start_row = scheduler->next_row;

    scheduler->next_row += scheduler->chunk_size;

    if (scheduler->next_row > scheduler->height) {
        scheduler->next_row = scheduler->height;
    }

    *end_row = scheduler->next_row;

    pthread_mutex_unlock(&scheduler->mutex);

    return 1;
}

static void *mandelbrot_thread_worker(void *arg) {
    MandelbrotThreadArgs *args = (MandelbrotThreadArgs *)arg;

    const MandelbrotConfig *cfg = args->cfg;
    MandelbrotImage *img = args->img;

    int width = cfg->width;
    int height = cfg->height;

    double x_scale = (cfg->x_max - cfg->x_min) / width;
    double y_scale = (cfg->y_max - cfg->y_min) / height;

    while (1) {
        int start_row;
        int end_row;

        if (!scheduler_get_next_chunk(args->scheduler,
                                      &start_row,
                                      &end_row)) {
            break;
        }

        for (int y = start_row; y < end_row; y++) {
            for (int x = 0; x < width; x++) {
                double cr = cfg->x_min + x * x_scale;
                double ci = cfg->y_min + y * y_scale;

                MandelbrotPointResult result =
                    mandelbrot_iterations(cr, ci, cfg->max_iter);

                int index = y * width + x;

                img->iterations[index] = result.iterations;
                img->smooth_values[index] = result.smooth_value;

                if (result.iterations < cfg->max_iter) {
                    args->local_histogram[result.iterations]++;
                }
            }
        }
    }
    return NULL;
}

void mandelbrot_compute_pthreads(const MandelbrotConfig *cfg, MandelbrotImage *img) {
    int thread_count = cfg->threads;

    /* Allocate thread structures */
    pthread_t *threads = malloc(sizeof(pthread_t) * thread_count);

    MandelbrotThreadArgs *args = malloc(sizeof(MandelbrotThreadArgs) * thread_count);

    int **local_histograms = malloc(sizeof(int *) * thread_count);

    if (!threads || !args || !local_histograms) {
        fprintf(stderr, "Failed to allocate thread data\n");

        free(threads);
        free(args);
        free(local_histograms);

        return;
    }
    
    /* Allocate per-thread histograms */
    for (int t = 0; t < thread_count; t++) {
        local_histograms[t] = calloc(cfg->max_iter, sizeof(int));

        if (!local_histograms[t]) {
            fprintf(stderr, "Failed to allocate thread histogram\n");

            for (int j = 0; j < t; j++) {
                free(local_histograms[j]);
            }

            free(local_histograms);
            free(args);
            free(threads);

            return;
        }
    }

    MandelbrotScheduler scheduler;

    scheduler.next_row = 0;
    scheduler.height = cfg->height;
    scheduler.chunk_size = cfg->chunk_size;
    
    if (pthread_mutex_init(&scheduler.mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialise scheduler mutex\n");

        for (int t = 0; t < thread_count; t++) {
            free(local_histograms[t]);
        }

        free(local_histograms);
        free(args);
        free(threads);

        return;
    }

    /* Create workers */
    for (int t = 0; t < thread_count; t++) {
        args[t].cfg = cfg;
        args[t].img = img;
        args[t].scheduler = &scheduler;
        args[t].local_histogram = local_histograms[t];

        pthread_create(&threads[t], NULL, mandelbrot_thread_worker, &args[t]);
    }

    /* Wait for workers */
    for (int t = 0; t < thread_count; t++) {
        pthread_join(threads[t], NULL);
    }

    /* Histogram reduction */
    for (int t = 0; t < thread_count; t++) {
        for (int i = 0; i < cfg->max_iter; i++) {
            img->histogram[i] += local_histograms[t][i];
        }
    }

    /* Clean up */
    pthread_mutex_destroy(&scheduler.mutex);

    for (int t = 0; t < thread_count; t++) {
        free(local_histograms[t]);
    }

    free(local_histograms);
    free(args);
    free(threads);
}
