/**
 * @file demosaic.c
 * @brief Bayer demosaicing implementation
 * 
 * Implements Bayer pattern RAW image demosaicing algorithms supporting four standard patterns:
 * - RGGB (Red-Green-Green-Blue)
 * - GRBG (Green-Red-Blue-Green)
 * - GBRG (Green-Blue-Red-Green) 
 * - BGGR (Blue-Green-Green-Red)
 * 
 * Algorithm uses bilinear interpolation to calculate missing color channels based on
 * pixel positions in the Bayer pattern.
 * 
 * @author Image Processor Team
 * @date 2025
 * @version 1.0
 */

#include "../include/image_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal function declarations
static uint8_t bilinear_interpolate(const uint8_t *data, uint32_t width, uint32_t height,
                                   float x, float y);
static void demosaic_pixel_rggb(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y);
static void demosaic_pixel_grbg(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y);
static void demosaic_pixel_gbrg(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y);
static void demosaic_pixel_bggr(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y);

// Public API implementation
int image_processor_init(void) {
    return 0;
}

void image_processor_cleanup(void) {
}

image_t* create_image(uint32_t width, uint32_t height, image_type_t type, uint32_t format_or_bpp) {
    if (width == 0 || height == 0) {
        return NULL;
    }

    image_t *image = (image_t*)malloc(sizeof(image_t));
    if (!image) {
        return NULL;
    }

    image->type = type;
    image->width = width;
    image->height = height;

    size_t data_size = 0;
    if (type == IMAGE_TYPE_RAW) {
        uint8_t bpp = (uint8_t)format_or_bpp;
        if (bpp == 0) {
            free(image);
            return NULL;
        }
        data_size = width * height * (bpp / 8);
        image->planes.raw.data = (uint8_t*)malloc(data_size);
        if (!image->planes.raw.data) {
            free(image);
            return NULL;
        }
        image->planes.raw.stride = width * (bpp / 8);
        image->planes.raw.bpp = bpp;
        memset(image->planes.raw.data, 0, data_size);
    } else if (type == IMAGE_TYPE_RGB) {
        data_size = width * height;
        image->planes.rgb.r = (uint8_t*)malloc(data_size);
        image->planes.rgb.g = (uint8_t*)malloc(data_size);
        image->planes.rgb.b = (uint8_t*)malloc(data_size);
        if (!image->planes.rgb.r || !image->planes.rgb.g || !image->planes.rgb.b) {
            if (image->planes.rgb.r) free(image->planes.rgb.r);
            if (image->planes.rgb.g) free(image->planes.rgb.g);
            if (image->planes.rgb.b) free(image->planes.rgb.b);
            free(image);
            return NULL;
        }
        memset(image->planes.rgb.r, 0, data_size);
        memset(image->planes.rgb.g, 0, data_size);
        memset(image->planes.rgb.b, 0, data_size);
    } else if (type == IMAGE_TYPE_YUV) {
        yuv_format_t format = (yuv_format_t)format_or_bpp;
        image->planes.yuv.format = format;
        size_t y_size = width * height;
        size_t u_size, v_size;
        switch (format) {
            case YUV_420P:
                u_size = v_size = (width / 2) * (height / 2);
                break;
            case YUV_422P:
                u_size = v_size = (width / 2) * height;
                break;
            case YUV_444P:
                u_size = v_size = y_size;
                break;
            default:
                free(image);
                return NULL;
        }
        image->planes.yuv.y = (uint8_t*)malloc(y_size);
        image->planes.yuv.u = (uint8_t*)malloc(u_size);
        image->planes.yuv.v = (uint8_t*)malloc(v_size);
        if (!image->planes.yuv.y || !image->planes.yuv.u || !image->planes.yuv.v) {
            if (image->planes.yuv.y) free(image->planes.yuv.y);
            if (image->planes.yuv.u) free(image->planes.yuv.u);
            if (image->planes.yuv.v) free(image->planes.yuv.v);
            free(image);
            return NULL;
        }
        memset(image->planes.yuv.y, 0, y_size);
        memset(image->planes.yuv.u, 128, u_size);
        memset(image->planes.yuv.v, 128, v_size);
    }

    return image;
}

void free_image(image_t *image) {
    if (!image) return;

    if (image->type == IMAGE_TYPE_RAW) {
        if (image->planes.raw.data) free(image->planes.raw.data);
    } else if (image->type == IMAGE_TYPE_RGB) {
        if (image->planes.rgb.r) free(image->planes.rgb.r);
        if (image->planes.rgb.g) free(image->planes.rgb.g);
        if (image->planes.rgb.b) free(image->planes.rgb.b);
    } else if (image->type == IMAGE_TYPE_YUV) {
        if (image->planes.yuv.y) free(image->planes.yuv.y);
        if (image->planes.yuv.u) free(image->planes.yuv.u);
        if (image->planes.yuv.v) free(image->planes.yuv.v);
    }
    free(image);
}

int demosaic_bayer(const image_t *raw_image,
                   bayer_pattern_t pattern,
                   image_t *rgb_image) {
    // Input validation
    if (!raw_image || raw_image->type != IMAGE_TYPE_RAW || !raw_image->planes.raw.data || 
        !rgb_image || rgb_image->type != IMAGE_TYPE_RGB) {
        return -1;
    }

    // Check dimension consistency
    if (raw_image->width != rgb_image->width ||
        raw_image->height != rgb_image->height) {
        return -2;
    }

    uint32_t width = raw_image->width;
    uint32_t height = raw_image->height;

    // Select processing function based on Bayer pattern
    void (*demosaic_func)(const image_t*, image_t*, uint32_t, uint32_t);

    switch (pattern) {
        case BAYER_RGGB:
            demosaic_func = demosaic_pixel_rggb;
            break;
        case BAYER_GRBG:
            demosaic_func = demosaic_pixel_grbg;
            break;
        case BAYER_GBRG:
            demosaic_func = demosaic_pixel_gbrg;
            break;
        case BAYER_BGGR:
            demosaic_func = demosaic_pixel_bggr;
            break;
        default:
            return -3;  // Unsupported Bayer pattern
    }

    // Process each pixel
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            demosaic_func(raw_image, rgb_image, x, y);
        }
    }

    return 0;
}

// Internal algorithm implementations
static uint8_t bilinear_interpolate(const uint8_t *data, uint32_t width, uint32_t height,
                                   float x, float y) {
    // Boundary check - use nearest neighbor for edge cases
    if (x < 0 || y < 0 || x >= width - 1 || y >= height - 1) {
        int ix = (int)roundf(x);
        int iy = (int)roundf(y);
        ix = ix < 0 ? 0 : (ix >= (int)width ? width - 1 : ix);
        iy = iy < 0 ? 0 : (iy >= (int)height ? height - 1 : iy);
        return data[iy * width + ix];
    }

    // Get coordinates of four neighboring pixels
    int x1 = (int)x;
    int y1 = (int)y;
    int x2 = x1 + 1;
    int y2 = y1 + 1;

    // Calculate offsets within pixel (0.0-1.0)
    float dx = x - x1;
    float dy = y - y1;

    // Get pixel values at four corners
    uint8_t q11 = data[y1 * width + x1];  // Top-left
    uint8_t q12 = data[y2 * width + x1];  // Bottom-left
    uint8_t q21 = data[y1 * width + x2];  // Top-right
    uint8_t q22 = data[y2 * width + x2];  // Bottom-right

    // Interpolate in x direction first
    float r1 = q11 * (1 - dx) + q21 * dx;  // Top edge interpolation
    float r2 = q12 * (1 - dx) + q22 * dx;  // Bottom edge interpolation
    
    // Interpolate in y direction for final result
    float result = r1 * (1 - dy) + r2 * dy;

    return (uint8_t)roundf(result);
}

static void demosaic_pixel_rggb(const image_t *raw, image_t *rgb,
                               uint32_t x, uint32_t y) {
    uint32_t width = raw->width;
    uint32_t height = raw->height;
    uint8_t *raw_data = raw->planes.raw.data;
    uint32_t idx = y * width + x;

    uint8_t r_val, g_val, b_val;

    if ((y % 2 == 0) && (x % 2 == 0)) {
        // Red pixel position
        r_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;

        int b_count = 0, b_sum = 0;
        if (x > 0 && y > 0) { b_sum += raw_data[idx - width - 1]; b_count++; }
        if (x < width - 1 && y > 0) { b_sum += raw_data[idx - width + 1]; b_count++; }
        if (x > 0 && y < height - 1) { b_sum += raw_data[idx + width - 1]; b_count++; }
        if (x < width - 1 && y < height - 1) { b_sum += raw_data[idx + width + 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;

    } else if ((y % 2 == 0) && (x % 2 == 1)) {
        // Green pixel (red row)
        g_val = raw_data[idx];
        int r_count = 0, r_sum = 0;
        if (x > 0) { r_sum += raw_data[idx - 1]; r_count++; }
        if (x < width - 1) { r_sum += raw_data[idx + 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;

        int b_count = 0, b_sum = 0;
        if (y > 0) { b_sum += raw_data[idx - width]; b_count++; }
        if (y < height - 1) { b_sum += raw_data[idx + width]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;

    } else if ((y % 2 == 1) && (x % 2 == 0)) {
        // Green pixel (blue row)
        g_val = raw_data[idx];
        int r_count = 0, r_sum = 0;
        if (y > 0) { r_sum += raw_data[idx - width]; r_count++; }
        if (y < height - 1) { r_sum += raw_data[idx + width]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;

        int b_count = 0, b_sum = 0;
        if (x > 0) { b_sum += raw_data[idx - 1]; b_count++; }
        if (x < width - 1) { b_sum += raw_data[idx + 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;

    } else {
        // Blue pixel position
        b_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;

        int r_count = 0, r_sum = 0;
        if (x > 0 && y > 0) { r_sum += raw_data[idx - width - 1]; r_count++; }
        if (x < width - 1 && y > 0) { r_sum += raw_data[idx - width + 1]; r_count++; }
        if (x > 0 && y < height - 1) { r_sum += raw_data[idx + width - 1]; r_count++; }
        if (x < width - 1 && y < height - 1) { r_sum += raw_data[idx + width + 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
    }

    rgb->planes.rgb.r[idx] = r_val;
    rgb->planes.rgb.g[idx] = g_val;
    rgb->planes.rgb.b[idx] = b_val;
}

static void demosaic_pixel_grbg(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y) {
    uint32_t width = raw->width;
    uint32_t height = raw->height;
    uint8_t *raw_data = raw->planes.raw.data;
    uint32_t idx = y * width + x;
    uint8_t r_val, g_val, b_val;

    if ((y % 2 == 0) && (x % 2 == 0)) {
        g_val = raw_data[idx];
        int r_count = 0, r_sum = 0;
        if (x < width - 1) { r_sum += raw_data[idx + 1]; r_count++; }
        if (x > 0) { r_sum += raw_data[idx - 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
        int b_count = 0, b_sum = 0;
        if (y > 0) { b_sum += raw_data[idx - width]; b_count++; }
        if (y < height - 1) { b_sum += raw_data[idx + width]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
    } else if ((y % 2 == 0) && (x % 2 == 1)) {
        r_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;
        int b_count = 0, b_sum = 0;
        if (x > 0 && y > 0) { b_sum += raw_data[idx - width - 1]; b_count++; }
        if (x < width - 1 && y > 0) { b_sum += raw_data[idx - width + 1]; b_count++; }
        if (x > 0 && y < height - 1) { b_sum += raw_data[idx + width - 1]; b_count++; }
        if (x < width - 1 && y < height - 1) { b_sum += raw_data[idx + width + 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
    } else if ((y % 2 == 1) && (x % 2 == 0)) {
        b_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;
        int r_count = 0, r_sum = 0;
        if (x > 0 && y > 0) { r_sum += raw_data[idx - width - 1]; r_count++; }
        if (x < width - 1 && y > 0) { r_sum += raw_data[idx - width + 1]; r_count++; }
        if (x > 0 && y < height - 1) { r_sum += raw_data[idx + width - 1]; r_count++; }
        if (x < width - 1 && y < height - 1) { r_sum += raw_data[idx + width + 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
    } else {
        g_val = raw_data[idx];
        int b_count = 0, b_sum = 0;
        if (x > 0) { b_sum += raw_data[idx - 1]; b_count++; }
        if (x < width - 1) { b_sum += raw_data[idx + 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
        int r_count = 0, r_sum = 0;
        if (y > 0) { r_sum += raw_data[idx - width]; r_count++; }
        if (y < height - 1) { r_sum += raw_data[idx + width]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
    }
    rgb->planes.rgb.r[idx] = r_val;
    rgb->planes.rgb.g[idx] = g_val;
    rgb->planes.rgb.b[idx] = b_val;
}

static void demosaic_pixel_gbrg(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y) {
    uint32_t width = raw->width;
    uint32_t height = raw->height;
    uint8_t *raw_data = raw->planes.raw.data;
    uint32_t idx = y * width + x;
    uint8_t r_val, g_val, b_val;

    if ((y % 2 == 0) && (x % 2 == 0)) {
        g_val = raw_data[idx];
        int b_count = 0, b_sum = 0;
        if (x < width - 1) { b_sum += raw_data[idx + 1]; b_count++; }
        if (x > 0) { b_sum += raw_data[idx - 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
        int r_count = 0, r_sum = 0;
        if (y > 0) { r_sum += raw_data[idx - width]; r_count++; }
        if (y < height - 1) { r_sum += raw_data[idx + width]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
    } else if ((y % 2 == 0) && (x % 2 == 1)) {
        b_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;
        int r_count = 0, r_sum = 0;
        if (x > 0 && y > 0) { r_sum += raw_data[idx - width - 1]; r_count++; }
        if (x < width - 1 && y > 0) { r_sum += raw_data[idx - width + 1]; r_count++; }
        if (x > 0 && y < height - 1) { r_sum += raw_data[idx + width - 1]; r_count++; }
        if (x < width - 1 && y < height - 1) { r_sum += raw_data[idx + width + 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
    } else if ((y % 2 == 1) && (x % 2 == 0)) {
        r_val = raw_data[idx];
        int g_count = 0, g_sum = 0;
        if (x > 0) { g_sum += raw_data[idx - 1]; g_count++; }
        if (x < width - 1) { g_sum += raw_data[idx + 1]; g_count++; }
        if (y > 0) { g_sum += raw_data[idx - width]; g_count++; }
        if (y < height - 1) { g_sum += raw_data[idx + width]; g_count++; }
        g_val = g_count > 0 ? (uint8_t)(g_sum / g_count) : 0;
        int b_count = 0, b_sum = 0;
        if (x > 0 && y > 0) { b_sum += raw_data[idx - width - 1]; b_count++; }
        if (x < width - 1 && y > 0) { b_sum += raw_data[idx - width + 1]; b_count++; }
        if (x > 0 && y < height - 1) { b_sum += raw_data[idx + width - 1]; b_count++; }
        if (x < width - 1 && y < height - 1) { b_sum += raw_data[idx + width + 1]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
    } else {
        g_val = raw_data[idx];
        int r_count = 0, r_sum = 0;
        if (x > 0) { r_sum += raw_data[idx - 1]; r_count++; }
        if (x < width - 1) { r_sum += raw_data[idx + 1]; r_count++; }
        r_val = r_count > 0 ? (uint8_t)(r_sum / r_count) : 0;
        int b_count = 0, b_sum = 0;
        if (y > 0) { b_sum += raw_data[idx - width]; b_count++; }
        if (y < height - 1) { b_sum += raw_data[idx + width]; b_count++; }
        b_val = b_count > 0 ? (uint8_t)(b_sum / b_count) : 0;
    }
    rgb->planes.rgb.r[idx] = r_val;
    rgb->planes.rgb.g[idx] = g_val;
    rgb->planes.rgb.b[idx] = b_val;
}

static void demosaic_pixel_bggr(const image_t *raw, image_t *rgb, uint32_t x, uint32_t y) {
    uint32_t width = raw->width;
    uint32_t idx = y * width + x;
    demosaic_pixel_rggb(raw, rgb, x, y);
    uint8_t temp = rgb->planes.rgb.r[idx];
    rgb->planes.rgb.r[idx] = rgb->planes.rgb.b[idx];
    rgb->planes.rgb.b[idx] = temp;
}