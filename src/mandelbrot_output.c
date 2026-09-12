// mandelbrot_output.c
#include "../include/mandelbrot.h"
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <stdint.h>

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

            uint8_t r;
            uint8_t g;
            uint8_t b;

            mandelbrot_colour_pixel(
                cfg,
                img,
                index,
                &r,
                &g,
                &b
            );

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
