#include "../include/mandelbrot.h"
#include <math.h>

typedef struct {
    double position;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} PaletteStop;

static const PaletteStop palette[] = {
    {0.00,   5,   8,  22},
    {0.15,  11,  31,  77},
    {0.35,  23,  74, 139},
    {0.55,  46, 134, 193},
    {0.72, 127, 219, 255},
    {0.88, 246, 231, 176},
    {1.00, 255, 248, 231}
};

static const size_t palette_size =
    sizeof(palette) / sizeof(palette[0]);

static uint8_t interpolate_channel(uint8_t a,
                                   uint8_t b,
                                   double t) {
    double value =
        (double)a + ((double)b - (double)a) * t;

    if (value < 0.0) {
        value = 0.0;
    }

    if (value > 255.0) {
        value = 255.0;
    }

    return (uint8_t)(value + 0.5);
}


static void palette_lookup(double value,
                           uint8_t *r,
                           uint8_t *g,
                           uint8_t *b) {
    if (value <= palette[0].position) {
        *r = palette[0].r;
        *g = palette[0].g;
        *b = palette[0].b;
        return;
    }

    if (value >= palette[palette_size - 1].position) {
        *r = palette[palette_size - 1].r;
        *g = palette[palette_size - 1].g;
        *b = palette[palette_size - 1].b;
        return;
    }

    for (size_t i = 0; i < palette_size - 1; i++) {
        const PaletteStop *left = &palette[i];
        const PaletteStop *right = &palette[i + 1];

        if (value >= left->position &&
            value <= right->position) {

            double range =
                right->position - left->position;

            double t =
                (value - left->position) / range;

            *r = interpolate_channel(left->r, right->r, t);
            *g = interpolate_channel(left->g, right->g, t);
            *b = interpolate_channel(left->b, right->b, t);

            return;
        }
    }
}


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

void mandelbrot_colour_pixel(
    const MandelbrotConfig *cfg,
    const MandelbrotImage *img,
    size_t index,
    uint8_t *r,
    uint8_t *g,
    uint8_t *b
) {
    int iter = img->iterations[index];

    double value;

    if (iter >= cfg->max_iter) {
        value = 0.0;
    }
    else {
        double smooth =
            img->smooth_values[index];

        int lower = (int)smooth;

        double fraction =
            smooth - (double)lower;

        if (lower < 0) {
            lower = 0;
            fraction = 0.0;
        }

        if (lower >= cfg->max_iter - 1) {
            value =
                img->cdf[cfg->max_iter - 1];
        }
        else {
            double c1 =
                img->cdf[lower];

            double c2 =
                img->cdf[lower + 1];

            value =
                c1 +
                fraction * (c2 - c1);
        }

        value =
            pow(value, cfg->gamma);
    }

    palette_lookup(
        value,
        r,
        g,
        b
    );
}

void mandelbrot_colour_pixel_preview(
    const MandelbrotConfig *cfg,
    const MandelbrotImage *img,
    size_t index,
    uint8_t *r,
    uint8_t *g,
    uint8_t *b
) {
    int iter = img->iterations[index];

    double value;

    if (iter >= cfg->max_iter) {
        value = 0.0;
    }
    else {
        double smooth =
            img->smooth_values[index];

        value =
            smooth / (double)cfg->max_iter;

        if (value < 0.0) {
            value = 0.0;
        }

        if (value > 1.0) {
            value = 1.0;
        }

        /*
         * Apply the same gamma setting so the preview
         * still resembles the final colour scheme.
         */
        value =
            pow(value, cfg->gamma);
    }

    palette_lookup(
        value,
        r,
        g,
        b
    );
}
