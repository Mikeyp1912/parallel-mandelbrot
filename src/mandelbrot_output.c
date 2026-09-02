// mandelbrot_output.c
#include "../include/mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <stdint.h>

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

int mandelbrot_write_png(const char *filename,
                         const MandelbrotConfig *cfg,
                         const MandelbrotImage *img) {
    FILE *fp = fopen(filename, "wb");

    if (!fp) {
        fprintf(stderr,
                "Error: could not open '%s' for writing\n",
                filename);
        return 1;
    }

    png_structp png =
        png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                NULL,
                                NULL,
                                NULL);

    if (!png) {
        fprintf(stderr,
                "Error: failed to create PNG write structure\n");
        fclose(fp);
        return 1;
    }

    png_infop info = png_create_info_struct(png);

    if (!info) {
        fprintf(stderr,
                "Error: failed to create PNG info structure\n");

        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return 1;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr,
                "Error: failed while writing PNG file\n");

        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return 1;
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        (png_uint_32)cfg->width,
        (png_uint_32)cfg->height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);

    size_t row_size =
        (size_t)cfg->width * 3;

    png_bytep row = malloc(row_size);

    if (!row) {
        fprintf(stderr,
                "Error: failed to allocate PNG row buffer\n");

        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return 1;
    }

    for (int y = 0; y < cfg->height; y++) {

        for (int x = 0; x < cfg->width; x++) {
            size_t index =
                (size_t)y * (size_t)cfg->width +
                (size_t)x;

            double value = img->pixels[index];

            uint8_t r;
            uint8_t g;
            uint8_t b;

            palette_lookup(value, &r, &g, &b);

            size_t pixel = (size_t)x * 3;

            row[pixel]     = r;
            row[pixel + 1] = g;
            row[pixel + 2] = b;
        }

        png_write_row(png, row);
    }

    png_write_end(png, NULL);

    free(row);

    png_destroy_write_struct(&png, &info);
    fclose(fp);

    return 0;
}
