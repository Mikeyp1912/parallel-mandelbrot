// mandelbrot.h
#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <stddef.h>

/*
 * mandelbrot.h
 *
 * Public interface for the Mandelbrot rendering system.
 *
 * This header defines:
 * - configuration data for a render
 * - image buffers used during computation
 * - function prototypes for setup, memory, computation, colouring, and output
 */

/* ------------------------------- Configuration ------------------------------ */


typedef enum {
    MANDELBROT_BACKEND_SERIAL,
    MANDELBROT_BACKEND_PTHREAD,
    MANDELBROT_BACKEND_AVX2,
    MANDELBROT_BACKEND_PTHREAD_AVX2
} MandelbrotBackend;


/*
 * Stores the user-controlled render settings.
 *
 * width, height:
 *		Output image resolution in pixels.
 *
 * max_iter:
 *		Maximum number of iterations used to test whether a point escapes.
 *
 * x_min, x_max, y_min, y_max:
 *		Bounds of the rectangular region in the complex plane to render.
 */
typedef struct {
	int width;
	int height;
	int max_iter;

	double x_min;
	double x_max;
	double y_min;
	double y_max;

    double center_x;
    double center_y;
    double zoom;

    double gamma;
    int no_output;
    const char *output_file;

    int threads;
    int chunk_size;

    MandelbrotBackend backend;
} MandelbrotConfig;
/* -------------------------------- Image Data -------------------------------- */

/*
 * Stores the allocated buffers used during and after computation.
 *
 * iterations:
 *      Escape iteration count for each pixel.
 *      Size: width * height
 *
 * histogram:
 *      Frequency count of iteration values.
 *      Size: max_iter
 *
 * smooth_values:
 *      Continuous escape value for each pixel, used for smooth colouring.
 *      Size: width * height
 *
 * cdf:
 *      Cumulative distribution of escaped iteration counts,
 *      used for histogram-based colouring.
 *      Size: max_iter
 */ 
typedef struct {
	int *iterations;
	int *histogram;
    double *smooth_values;
    double *cdf;
} MandelbrotImage;

/* config usage helper */
void mandelbrot_print_usage(const char *prog_name);

/* ---------------------------- Config Functions --------------------------- */

/*
 * Fills a MandelbrotConfig struct with sensible default values.
 */
void mandelbrot_set_defaults(MandelbrotConfig *cfg);

/*
 * Parses command-line arguments and updates the config.
 *
 * Returns:
 *     0 on success
 *     non-zero on invalid input
 */
int mandelbrot_parse_args(MandelbrotConfig *cfg, int argc, char *argv[]);

/* ---------------------------- Memory Functions --------------------------- */

/*
 * Allocates memory for the image buffers based on the supplied config.
 *
 * Returns:
 *     0 on success
 *     non-zero on allocation failure
 */
int mandelbrot_image_init(MandelbrotImage *img, const MandelbrotConfig *cfg);

/*
 * Frees any memory owned by the image buffers.
 */
void mandelbrot_image_free(MandelbrotImage *img);

typedef struct {
    int iterations;
    double smooth_value;
} MandelbrotPointResult;

/* -------------------------- Computation Functions ------------------------ */

/*
 * Computes the Mandelbrot escape iteration count for a single complex point.
 *
 * cr:
 *     Real part of the complex coordinate.
 *
 * ci:
 *     Imaginary part of the complex coordinate.
 *
 * max_iter:
 *     Maximum number of iterations allowed.
 *
 * Returns:
 *     The number of iterations before escape, or max_iter if the point
 *     does not escape within the limit.
 */
MandelbrotPointResult mandelbrot_iterations(double cr, double ci, int max_iter);

/*
 * Computes the Mandelbrot iteration count for every pixel in the image
 * using the serial implementation.
 */
void mandelbrot_compute_serial(const MandelbrotConfig *cfg, MandelbrotImage *img);

/*
 *
 */
int mandelbrot_compute_pthreads(const MandelbrotConfig *cfg, MandelbrotImage *img);


void mandelbrot_compute_avx2(const MandelbrotConfig *cfg, MandelbrotImage *img);

int mandelbrot_compute_pthreads_avx2(const MandelbrotConfig *cfg, MandelbrotImage *img);

void mandelbrot_compute_row_avx2(const MandelbrotConfig *cfg,
                                 MandelbrotImage *img,
                                 int y,
                                 int *histogram);

/* --------------------------- Colouring Functions ------------------------- */

int mandelbrot_build_colour_cdf(const MandelbrotConfig *cfg,
                                MandelbrotImage *img);

/* ----------------------------- Output Functions -------------------------- */

int mandelbrot_write_png(const char *filename,
                         const MandelbrotConfig *cfg,
                         const MandelbrotImage *img);

#endif
