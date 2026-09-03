// mandelbrot_core.c
#include "../include/mandelbrot.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

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
    cfg->output_file = "plot/mandel.png";

    cfg->threads = 1;
    cfg->chunk_size = 1;

    cfg->backend = MANDELBROT_BACKEND_SERIAL;
}

void mandelbrot_print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("\n");
    printf("General Options:\n");
    printf("  -h, --help            Show this help message\n");
    printf("  --width <pixels>      Image width\n");
    printf("  --height <pixels>     Image height\n");
    printf("  --iterations <count>  Maximum iterations\n");
    printf("  --gamma <value>       Colour gamma correction\n");
    printf("  --no-output           Skip colouring and output file generation\n");
    printf("  --output <file>       PNG output filename\n");
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
    printf("\nRendering Backend Options:\n");
    printf("  --backend <name>      Rendering backend\n");
    printf("                        Available: full, seahorse, deep-zoom\n");
    printf("\nPreset Options:\n");
    printf("  --preset <name>       Use a named render preset\n");
    printf("                        Available: full, seahorse, deep-zoom\n");
}

static int parse_int(const char *text, int *value) {
    char *end;
    errno = 0;

    long result = strtol(text, &end, 10);

    if (errno != 0 ||
            end == text ||
            *end != '\0' ||
            result < INT_MIN ||
            result > INT_MAX) {
        return 1;
    }

    *value = (int)result;
    return 0;
}

static int parse_double(const char *text, double *value) {
    char *end;
    errno = 0;

    double result = strtod(text, &end);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        !isfinite(result)) {
        return 1;
    }

    *value = result;
    return 0;
}

int mandelbrot_parse_args(MandelbrotConfig *cfg, int argc, char *argv[]) {
    int bounds_mode_used = 0;
    int center_zoom_mode_used = 0;
    int error_count = 0;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --width\n");
                error_count++;
                continue;
            }

            if (parse_int(argv[++i], &cfg->width) != 0) {
                fprintf(stderr, "Error: invalid width\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --height\n");
                error_count++;
                continue;
            }

            if (parse_int(argv[++i], &cfg->height) != 0) {
                fprintf(stderr, "Error: invalid height\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--iterations") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --iterations\n");
                error_count++;
                continue;
            }

            if (parse_int(argv[++i], &cfg->max_iter) != 0) {
                fprintf(stderr, "Error: invalid iteration count\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--gamma") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --gamma\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->gamma) != 0) {
                fprintf(stderr, "Error: invalid gamma value\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--xmin") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --xmin\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->x_min) != 0) {
                fprintf(stderr, "Error: invalid xmin value\n");
                error_count++;
            }

            bounds_mode_used = 1;
        }

        else if (strcmp(argv[i], "--xmax") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --xmax\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->x_max) != 0) {
                fprintf(stderr, "Error: invalid xmax value\n");
                error_count++;
            }

            bounds_mode_used = 1;
        }

        else if (strcmp(argv[i], "--ymin") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --ymin\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->y_min) != 0) {
                fprintf(stderr, "Error: invalid ymin value\n");
                error_count++;
            }

            bounds_mode_used = 1;
        }

        else if (strcmp(argv[i], "--ymax") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --ymax\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->y_max) != 0) {
                fprintf(stderr, "Error: invalid ymax value\n");
                error_count++;
            }

            bounds_mode_used = 1;
        }

        else if (strcmp(argv[i], "--center-x") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --center-x\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->center_x) != 0) {
                fprintf(stderr, "Error: invalid center-x value\n");
                error_count++;
            }

            center_zoom_mode_used = 1;
        }

        else if (strcmp(argv[i], "--center-y") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --center-y\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->center_y) != 0) {
                fprintf(stderr, "Error: invalid center-y value\n");
                error_count++;
            }

            center_zoom_mode_used = 1;
        }

        else if (strcmp(argv[i], "--zoom") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --zoom\n");
                error_count++;
                continue;
            }

            if (parse_double(argv[++i], &cfg->zoom) != 0) {
                fprintf(stderr, "Error: invalid zoom value\n");
                error_count++;
            }

            center_zoom_mode_used = 1;
        }

        else if (strcmp(argv[i], "--threads") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --threads\n");
                error_count++;
                continue;
            }

            if (parse_int(argv[++i], &cfg->threads) != 0) {
                fprintf(stderr, "Error: invalid thread count\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--chunk-size") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --chunk-size\n");
                error_count++;
                continue;
            }

            if (parse_int(argv[++i], &cfg->chunk_size) != 0) {
                fprintf(stderr, "Error: invalid chunk size\n");
                error_count++;
            }
        }

        else if (strcmp(argv[i], "--no-output") == 0) {
            cfg->no_output = 1;
        }

        else if (strcmp(argv[i], "--help") == 0 ||
                 strcmp(argv[i], "-h") == 0) {
            mandelbrot_print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "--preset") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --preset\n");
                error_count++;
                continue;
            }

            const char *preset = argv[++i];

            if (strcmp(preset, "full") == 0) {
                cfg->center_x = -0.5;
                cfg->center_y = 0.0;
                cfg->zoom = 1.0;

                cfg->width = 1500;
                cfg->height = 1500;
                cfg->max_iter = 2000;
                cfg->gamma = 4.0;

                center_zoom_mode_used = 1;
            }

            else if (strcmp(preset, "seahorse") == 0) {
                cfg->center_x = -0.743643887;
                cfg->center_y = 0.131825904;
                cfg->zoom = 100.0;

                cfg->width = 1500;
                cfg->height = 1500;
                cfg->max_iter = 5000;
                cfg->gamma = 3.5;

                center_zoom_mode_used = 1;
            }

            else if (strcmp(preset, "deep-zoom") == 0) {
                cfg->center_x = -0.743643887;
                cfg->center_y = 0.131825904;
                cfg->zoom = 1000.0;

                cfg->width = 2000;
                cfg->height = 2000;
                cfg->max_iter = 10000;
                cfg->gamma = 3.5;

                center_zoom_mode_used = 1;
            }

            else {
                fprintf(stderr,
                        "Error: unknown preset '%s'\n",
                        preset);
                error_count++;
            }
        }
        else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value for --output\n");
                error_count++;
                continue;
            }

            cfg->output_file = argv[++i];
        }
        else if (strcmp(argv[i], "--backend") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr,
                        "Error: --backend requires a value\n");
                error_count++;
                continue;
            }

            const char *backend = argv[++i];

            if (strcmp(backend, "serial") == 0) {
                cfg->backend = MANDELBROT_BACKEND_SERIAL;
            }
            else if (strcmp(backend, "pthread") == 0) {
                cfg->backend = MANDELBROT_BACKEND_PTHREAD;
            }
            else if (strcmp(backend, "avx2") == 0) {
                cfg->backend = MANDELBROT_BACKEND_AVX2;
            }
            else if (strcmp(backend, "pthread-avx2") == 0) {
                cfg->backend = MANDELBROT_BACKEND_PTHREAD_AVX2;
            }
            else {
                fprintf(stderr,
                        "Error: unknown backend '%s'\n",
                        backend);
                error_count++;
            }
        }
        else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            error_count++;

            if (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
            }
        }
    }


    /*
     * Validate general configuration.
     */

    if (cfg->width <= 0) {
        fprintf(stderr, "Error: width must be greater than 0\n");
        error_count++;
    }

    if (cfg->height <= 0) {
        fprintf(stderr, "Error: height must be greater than 0\n");
        error_count++;
    }

    if (cfg->max_iter <= 0) {
        fprintf(stderr, "Error: iterations must be greater than 0\n");
        error_count++;
    }

    if (cfg->gamma <= 0.0) {
        fprintf(stderr, "Error: gamma must be greater than 0\n");
        error_count++;
    }

    if (cfg->threads <= 0) {
        fprintf(stderr, "Error: threads must be greater than 0\n");
        error_count++;
    }

    if (cfg->chunk_size <= 0) {
        fprintf(stderr, "Error: chunk size must be greater than 0\n");
        error_count++;
    }


    /*
     * Validate view mode.
     */

    if (bounds_mode_used && center_zoom_mode_used) {
        fprintf(stderr,
                "Error: cannot combine manual bounds with "
                "center/zoom options\n");
        error_count++;
    }

    if (cfg->zoom <= 0.0) {
        fprintf(stderr, "Error: zoom must be greater than 0\n");
        error_count++;
    }


    /*
     * Convert centre/zoom coordinates into explicit bounds.
     *
     * Only do this when the selected view mode is valid and
     * the zoom value is usable.
     */

    if (center_zoom_mode_used &&
        !bounds_mode_used &&
        cfg->zoom > 0.0) {

        double x_span = 3.0 / cfg->zoom;

        double aspect =
            (double)cfg->height / (double)cfg->width;

        double y_span = x_span * aspect;

        cfg->x_min = cfg->center_x - x_span / 2.0;
        cfg->x_max = cfg->center_x + x_span / 2.0;

        cfg->y_min = cfg->center_y - y_span / 2.0;
        cfg->y_max = cfg->center_y + y_span / 2.0;
    }


    /*
     * Validate final complex-plane bounds.
     */

    if (cfg->x_min >= cfg->x_max) {
        fprintf(stderr, "Error: xmin must be less than xmax\n");
        error_count++;
    }

    if (cfg->y_min >= cfg->y_max) {
        fprintf(stderr, "Error: ymin must be less than ymax\n");
        error_count++;
    }


    /*
     * Return failure if any errors were encountered.
     */

    if (error_count > 0) {
        return 1;
    }

    if (cfg->threads > 1 &&
        cfg->backend == MANDELBROT_BACKEND_SERIAL) {
        cfg->backend = MANDELBROT_BACKEND_PTHREAD;
    }

    return 0;
}

int mandelbrot_image_init(MandelbrotImage *img,
                          const MandelbrotConfig *cfg) {
    size_t width = (size_t)cfg->width;
    size_t height = (size_t)cfg->height;

    /*
     * Check for overflow before calculating the total
     * number of pixels.
     */
    if (height != 0 && width > SIZE_MAX / height) {
        fprintf(stderr, "Error: image dimensions are too large\n");
        return 1;
    }

    size_t total_pixels = width * height;

    if (total_pixels > SIZE_MAX / sizeof(double)) {
        fprintf(stderr, "Error: image dimensions are too large\n");
        return 1;
    }

    /*
     * Initialise pointers so cleanup is always safe.
     */
    img->iterations = NULL;
    img->histogram = NULL;
    img->smooth_values = NULL;
    img->cdf = NULL;

    img->iterations =
        malloc(sizeof(int) * total_pixels);

    img->histogram =
        calloc((size_t)cfg->max_iter, sizeof(int));

    img->smooth_values =
        malloc(sizeof(double) * total_pixels);

    img->cdf = malloc(sizeof(double) * (size_t)cfg->max_iter);

    if (!img->iterations ||
        !img->histogram ||
        !img->smooth_values ||
        !img->cdf) {

        fprintf(stderr,
                "Error: failed to allocate Mandelbrot image buffers\n");

        mandelbrot_image_free(img);
        return 1;
    }
    return 0;
}


void mandelbrot_image_free(MandelbrotImage *img) {
	free(img->iterations);
	free(img->histogram);
    free(img->smooth_values);
    free(img->cdf);

	img->iterations = NULL;
	img->histogram = NULL;
    img->smooth_values = NULL;
    img->cdf = NULL;
}

static inline int mandelbrot_known_interior(double cr, double ci) {
    /*
     * Main cardioid test.
     *
     * q = (x - 1/4)^2 + y^2
     * Point is inside the cardioid if:
     *
     * q * (q + (x - 1/4)) <= 1/4 * y^2
     */
    double x = cr - 0.25;
    double y2 = ci * ci;
    double q = x * x + y2;

    if (q * (q + x) <= 0.25 * y2) {
        return 1;
    }

    /*
     * Period-2 bulb.
     *
     * Center = (-1, 0)
     * Radius = 1/4
     */
    double bulb_x = cr + 1.0;

    if (bulb_x * bulb_x + y2 <= 0.0625) {
        return 1;
    }

    return 0;
}

MandelbrotPointResult mandelbrot_iterations(double cr, double ci, int max_iter) {
    MandelbrotPointResult result;

    if (mandelbrot_known_interior(cr, ci)) {
        result.iterations = max_iter;
        result.smooth_value = 0.0;
        return result;
    }

	double zr = 0.0;
	double zi = 0.0;
	int iter = 0;

	while (zr * zr + zi * zi <= 4.0 && iter < max_iter) {
		double temp = zr * zr - zi * zi + cr;
		zi = 2.0 * zr * zi + ci;
		zr = temp;

		iter++;
	}

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

            if (cfg->backend == MANDELBROT_BACKEND_PTHREAD_AVX2) {
                mandelbrot_compute_row_avx2(
                    cfg,
                    img,
                    y,
                    args->local_histogram
                );

                continue;
            }

            for (int x = 0; x < width; x++) {
                double cr = cfg->x_min + x * x_scale;
                double ci = cfg->y_min + y * y_scale;

                MandelbrotPointResult result =
                    mandelbrot_iterations(
                        cr,
                        ci,
                        cfg->max_iter
                    );

                int index = y * width + x;

                img->iterations[index] =
                    result.iterations;

                img->smooth_values[index] =
                    result.smooth_value;

                if (result.iterations < cfg->max_iter) {
                    args->local_histogram[result.iterations]++;
                }
            }
        }
    }

    return NULL;
}

int mandelbrot_compute_pthreads(const MandelbrotConfig *cfg,
                                MandelbrotImage *img) {
    int thread_count = cfg->threads;
    int created_threads = 0;
    int status = 0;

    pthread_t *threads =
        malloc(sizeof(pthread_t) * (size_t)thread_count);

    MandelbrotThreadArgs *args =
        malloc(sizeof(MandelbrotThreadArgs) * (size_t)thread_count);

    int **local_histograms =
        calloc((size_t)thread_count, sizeof(int *));

    if (!threads || !args || !local_histograms) {
        fprintf(stderr, "Error: failed to allocate thread data\n");

        free(threads);
        free(args);
        free(local_histograms);

        return 1;
    }


    /*
     * Allocate one private histogram per worker.
     */

    for (int t = 0; t < thread_count; t++) {
        local_histograms[t] =
            calloc((size_t)cfg->max_iter, sizeof(int));

        if (!local_histograms[t]) {
            fprintf(stderr,
                    "Error: failed to allocate histogram for thread %d\n",
                    t);

            for (int j = 0; j < t; j++) {
                free(local_histograms[j]);
            }

            free(local_histograms);
            free(args);
            free(threads);

            return 1;
        }
    }


    /*
     * Initialise the shared dynamic scheduler.
     */

    MandelbrotScheduler scheduler;

    scheduler.next_row = 0;
    scheduler.height = cfg->height;
    scheduler.chunk_size = cfg->chunk_size;

    int mutex_result =
        pthread_mutex_init(&scheduler.mutex, NULL);

    if (mutex_result != 0) {
        fprintf(stderr,
                "Error: failed to initialise scheduler mutex: %s\n",
                strerror(mutex_result));

        for (int t = 0; t < thread_count; t++) {
            free(local_histograms[t]);
        }

        free(local_histograms);
        free(args);
        free(threads);

        return 1;
    }


    /*
     * Create worker threads.
     */

    for (int t = 0; t < thread_count; t++) {
        args[t].cfg = cfg;
        args[t].img = img;
        args[t].scheduler = &scheduler;
        args[t].local_histogram = local_histograms[t];

        int create_result =
            pthread_create(&threads[t],
                           NULL,
                           mandelbrot_thread_worker,
                           &args[t]);

        if (create_result != 0) {
            fprintf(stderr,
                    "Error: failed to create thread %d: %s\n",
                    t,
                    strerror(create_result));

            status = 1;
            break;
        }

        created_threads++;
    }


    /*
     * Wait only for workers that were successfully created.
     */

    for (int t = 0; t < created_threads; t++) {
        int join_result =
            pthread_join(threads[t], NULL);

        if (join_result != 0) {
            fprintf(stderr,
                    "Error: failed to join thread %d: %s\n",
                    t,
                    strerror(join_result));

            status = 1;
        }
    }


    /*
     * Only use the result if all requested workers were
     * successfully created and joined.
     */

    if (status == 0 && created_threads == thread_count) {
        for (int t = 0; t < thread_count; t++) {
            for (int i = 0; i < cfg->max_iter; i++) {
                img->histogram[i] += local_histograms[t][i];
            }
        }
    }
    else {
        status = 1;
    }


    /*
     * Clean up scheduler and thread resources.
     */

    int destroy_result =
        pthread_mutex_destroy(&scheduler.mutex);

    if (destroy_result != 0) {
        fprintf(stderr,
                "Error: failed to destroy scheduler mutex: %s\n",
                strerror(destroy_result));

        status = 1;
    }

    for (int t = 0; t < thread_count; t++) {
        free(local_histograms[t]);
    }

    free(local_histograms);
    free(args);
    free(threads);

    return status;
}


int mandelbrot_compute_pthreads_avx2(
    const MandelbrotConfig *cfg,
    MandelbrotImage *img) {

    return mandelbrot_compute_pthreads(cfg, img);
}
