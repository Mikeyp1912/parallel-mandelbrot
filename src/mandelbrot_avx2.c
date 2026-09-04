#include "../include/mandelbrot.h"
#include <immintrin.h>
#include <math.h>
#include <stdint.h>


static int mandelbrot_is_known_interior(double cr, double ci) {
    double ci2 = ci * ci;

    double x = cr - 0.25;
    double q = x * x + ci2;

    if (q * (q + x) <= 0.25 * ci2) {
        return 1;
    }

    x = cr + 1.0;

    if (x * x + ci2 <= 0.0625) {
        return 1;
    }

    return 0;
}

static void mandelbrot_iterations_avx2_4(
    const double cr_values[4],
    double ci,
    int max_iter,
    int iterations[4],
    double smooth_values[4]) {

    int interior[4];

    for (int lane = 0; lane < 4; lane++) {
        interior[lane] =
            mandelbrot_is_known_interior(cr_values[lane], ci);
    }

    __m256d cr = _mm256_loadu_pd(cr_values);
    __m256d ci_vec = _mm256_set1_pd(ci);

    __m256d zr = _mm256_setzero_pd();
    __m256d zi = _mm256_setzero_pd();

    /*
     * active contains one mask bit per double lane.
     *
     * Interior pixels start inactive because we already know
     * they belong to the Mandelbrot set.
     */
    __m256d active = _mm256_castsi256_pd(
        _mm256_set_epi64x(
            interior[3] ? 0LL : -1LL,
            interior[2] ? 0LL : -1LL,
            interior[1] ? 0LL : -1LL,
            interior[0] ? 0LL : -1LL
        )
    );

    __m256i counts = _mm256_setzero_si256();

    const __m256d four =
        _mm256_set1_pd(4.0);

    const __m256i one =
        _mm256_set1_epi64x(1);

    for (int step = 0;
         step < max_iter && _mm256_movemask_pd(active) != 0;
         step++) {

        /*
         * Mandelbrot:
         *
         * new_zr = zr² - zi² + cr
         * new_zi = 2*zr*zi + ci
         */

        __m256d zr2 =
            _mm256_mul_pd(zr, zr);

        __m256d zi2 =
            _mm256_mul_pd(zi, zi);

        __m256d zrzi =
            _mm256_mul_pd(zr, zi);

        __m256d new_zr =
            _mm256_add_pd(
                _mm256_sub_pd(zr2, zi2),
                cr
            );

        __m256d new_zi =
            _mm256_add_pd(
                _mm256_add_pd(zrzi, zrzi),
                ci_vec
            );

        /*
         * Only update lanes that are still active.
         *
         * Escaped pixels retain the z value at which
         * they escaped so we can calculate smooth colour
         * values later.
         */
        zr = _mm256_blendv_pd(zr, new_zr, active);
        zi = _mm256_blendv_pd(zi, new_zi, active);

        /*
         * Add one iteration to each active lane.
         */
        __m256i active_int =
            _mm256_castpd_si256(active);

        __m256i increment =
            _mm256_and_si256(active_int, one);

        counts =
            _mm256_add_epi64(counts, increment);

        /*
         * Determine which lanes remain inside |z| <= 2.
         */
        zr2 = _mm256_mul_pd(zr, zr);
        zi2 = _mm256_mul_pd(zi, zi);

        __m256d magnitude2 =
            _mm256_add_pd(zr2, zi2);

        __m256d inside_radius =
            _mm256_cmp_pd(
                magnitude2,
                four,
                _CMP_LE_OQ
            );

        active =
            _mm256_and_pd(active, inside_radius);
    }

    int64_t count_values[4];
    double zr_values[4];
    double zi_values[4];

    _mm256_storeu_si256(
        (__m256i *)count_values,
        counts
    );

    _mm256_storeu_pd(zr_values, zr);
    _mm256_storeu_pd(zi_values, zi);

    for (int lane = 0; lane < 4; lane++) {

        if (interior[lane]) {
            iterations[lane] = max_iter;
            smooth_values[lane] = 0.0;
            continue;
        }

        iterations[lane] =
            (int)count_values[lane];

        if (iterations[lane] >= max_iter) {
            smooth_values[lane] = 0.0;
        }
        else {
            double magnitude =
                sqrt(
                    zr_values[lane] * zr_values[lane] +
                    zi_values[lane] * zi_values[lane]
                );

            smooth_values[lane] =
                iterations[lane] +
                1.0 -
                log(log(magnitude)) / log(2.0);
        }
    }
}

void mandelbrot_compute_row_range_avx2(
            const MandelbrotConfig *cfg,
            MandelbrotImage *img,
            int y,
            int x_start,
            int x_end,
            int *histogram
        ) {
    double x_scale =
        (cfg->x_max - cfg->x_min) /
        (double)cfg->width;

    double y_scale =
        (cfg->y_max - cfg->y_min) /
        (double)cfg->height;

    double ci =
        cfg->y_min +
        (double)y * y_scale;

    int x = x_start;

    for (; x + 3 < x_end; x += 4) {

        double cr_values[4] = {
            cfg->x_min + (double)(x + 0) * x_scale,
            cfg->x_min + (double)(x + 1) * x_scale,
            cfg->x_min + (double)(x + 2) * x_scale,
            cfg->x_min + (double)(x + 3) * x_scale
        };

        int iter_values[4];
        double smooth_values[4];

        mandelbrot_iterations_avx2_4(
            cr_values,
            ci,
            cfg->max_iter,
            iter_values,
            smooth_values
        );

        for (int lane = 0; lane < 4; lane++) {

            size_t index =
                (size_t)y * (size_t)cfg->width +
                (size_t)(x + lane);

            img->iterations[index] =
                iter_values[lane];

            img->smooth_values[index] =
                smooth_values[lane];

            if (iter_values[lane] < cfg->max_iter) {
                histogram[iter_values[lane]]++;
            }
        }
    }
    /*
    * Handle the final 1-3 pixels in this range if its
     * width is not divisible by four.
     */
    for (; x < x_end; x++) {

        double cr =
            cfg->x_min +
            (double)x * x_scale;

        MandelbrotPointResult result =
            mandelbrot_iterations(
                cr,
                ci,
                cfg->max_iter,
                cfg->periodicity_check
            );

        size_t index =
            (size_t)y * (size_t)cfg->width +
            (size_t)x;

        img->iterations[index] =
            result.iterations;

        img->smooth_values[index] =
            result.smooth_value;

        if (result.iterations < cfg->max_iter) {
            histogram[result.iterations]++;
        }
    }
}

void mandelbrot_compute_row_avx2(
    const MandelbrotConfig *cfg,
    MandelbrotImage *img,
    int y,
    int *histogram
) {
    mandelbrot_compute_row_range_avx2(
        cfg,
        img,
        y,
        0,
        cfg->width,
        histogram
    );
}

void mandelbrot_compute_avx2(const MandelbrotConfig *cfg,
                             MandelbrotImage *img) {
    for (int y = 0; y < cfg->height; y++) {
        mandelbrot_compute_row_avx2(
            cfg,
            img,
            y,
            img->histogram
        );
    }
}
