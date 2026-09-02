#include "../include/mandelbrot.h"

int mandelbrot_build_colour_cdf(const MandelbrotConfig *cfg,
                                MandelbrotImage *img) {
    long long escaped_pixels = 0;

    for (int i = 0; i < cfg->max_iter; i++) {
        escaped_pixels += img->histogram[i];
    }

    if (escaped_pixels == 0) {
        for (int i = 0; i < cfg->max_iter; i++) {
            img->cdf[i] = 0.0;
        }

        return 0;
    }

    long long cumulative = 0;

    for (int i = 0; i < cfg->max_iter; i++) {
        cumulative += img->histogram[i];

        img->cdf[i] =
            (double)cumulative /
            (double)escaped_pixels;
    }

    return 0;
}
