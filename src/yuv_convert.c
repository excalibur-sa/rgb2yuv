/**
 * @file yuv_convert.c
 * @brief RGB to YUV conversion implementation
 * 
 * Implements high-performance RGB to YUV format conversion using fixed-point arithmetic.
 * Supports YUV420P, YUV422P, and YUV444P output formats.
 * 
 * Optimization techniques:
 * - Fixed-point arithmetic (coefficients × 1024) instead of floating-point
 * - Inline functions for critical calculations
 * - Memory layout optimization for cache efficiency
 * 
 * @author Image Processor Team
 * @date 2025
 * @version 1.0
 */

#include "../include/image_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fixed-point coefficients (multiplied by 1024 for precision)
#define COEF_Y_R    299  // 0.299 * 1024
#define COEF_Y_G    587  // 0.587 * 1024  
#define COEF_Y_B    114  // 0.114 * 1024
#define COEF_U_R   -169  // -0.168736 * 1024
#define COEF_U_G   -331  // -0.331264 * 1024
#define COEF_U_B    500  // 0.5 * 1024
#define COEF_V_R    500  // 0.5 * 1024
#define COEF_V_G   -419  // -0.418688 * 1024
#define COEF_V_B   -81   // -0.081312 * 1024

// Y offset (16) and UV offset (128) in fixed-point
#define Y_OFFSET    16
#define UV_OFFSET   128

// Internal function declarations
static inline uint8_t calc_y(uint8_t r, uint8_t g, uint8_t b);
static inline uint8_t calc_u(uint8_t r, uint8_t g, uint8_t b);
static inline uint8_t calc_v(uint8_t r, uint8_t g, uint8_t b);
static void convert_y_plane(const image_t *rgb, image_t *yuv);
static void rgb_to_yuv420p(const image_t *rgb, image_t *yuv);
static void rgb_to_yuv422p(const image_t *rgb, image_t *yuv);
static void rgb_to_yuv444p(const image_t *rgb, image_t *yuv);

// Inline calculation functions
static inline uint8_t calc_y(uint8_t r, uint8_t g, uint8_t b) {
    int y = (COEF_Y_R * r + COEF_Y_G * g + COEF_Y_B * b) / 1024 + Y_OFFSET;
    return y < 0 ? 0 : (y > 255 ? 255 : (uint8_t)y);
}

static inline uint8_t calc_u(uint8_t r, uint8_t g, uint8_t b) {
    int u = (COEF_U_R * r + COEF_U_G * g + COEF_U_B * b) / 1024 + UV_OFFSET;
    return u < 0 ? 0 : (u > 255 ? 255 : (uint8_t)u);
}

static inline uint8_t calc_v(uint8_t r, uint8_t g, uint8_t b) {
    int v = (COEF_V_R * r + COEF_V_G * g + COEF_V_B * b) / 1024 + UV_OFFSET;
    return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v);
}

static void convert_y_plane(const image_t *rgb, image_t *yuv) {
    uint32_t size = rgb->width * rgb->height;
    for (uint32_t i = 0; i < size; i++) {
        uint8_t r = rgb->planes.rgb.r[i], g = rgb->planes.rgb.g[i], b = rgb->planes.rgb.b[i];
        yuv->planes.yuv.y[i] = calc_y(r, g, b);
    }
}

static void rgb_to_yuv420p(const image_t *rgb, image_t *yuv) {
    uint32_t width = rgb->width;
    uint32_t height = rgb->height;
    uint32_t uv_width = width / 2;
    uint32_t uv_height = height / 2;

    convert_y_plane(rgb, yuv);

    for (uint32_t y = 0; y < uv_height; y++) {
        for (uint32_t x = 0; x < uv_width; x++) {
            uint32_t src_x = x * 2;
            uint32_t src_y = y * 2;
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;

            for (int dy = 0; dy < 2 && (src_y + dy) < height; dy++) {
                for (int dx = 0; dx < 2 && (src_x + dx) < width; dx++) {
                    uint32_t idx = (src_y + dy) * width + (src_x + dx);
                    r_sum += rgb->planes.rgb.r[idx];
                    g_sum += rgb->planes.rgb.g[idx];
                    b_sum += rgb->planes.rgb.b[idx];
                    count++;
                }
            }
            uint8_t r = r_sum / count, g = g_sum / count, b = b_sum / count;
            uint32_t uv_idx = y * uv_width + x;
            yuv->planes.yuv.u[uv_idx] = calc_u(r, g, b);
            yuv->planes.yuv.v[uv_idx] = calc_v(r, g, b);
        }
    }
}

static void rgb_to_yuv422p(const image_t *rgb, image_t *yuv) {
    uint32_t width = rgb->width;
    uint32_t height = rgb->height;
    uint32_t uv_width = width / 2;

    convert_y_plane(rgb, yuv);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < uv_width; x++) {
            uint32_t src_x = x * 2;
            int r_sum = rgb->planes.rgb.r[y * width + src_x];
            int g_sum = rgb->planes.rgb.g[y * width + src_x];
            int b_sum = rgb->planes.rgb.b[y * width + src_x];
            int count = 1;

            if (src_x + 1 < width) {
                r_sum += rgb->planes.rgb.r[y * width + src_x + 1];
                g_sum += rgb->planes.rgb.g[y * width + src_x + 1];
                b_sum += rgb->planes.rgb.b[y * width + src_x + 1];
                count++;
            }
            uint8_t r = r_sum / count, g = g_sum / count, b = b_sum / count;
            uint32_t uv_idx = y * uv_width + x;
            yuv->planes.yuv.u[uv_idx] = calc_u(r, g, b);
            yuv->planes.yuv.v[uv_idx] = calc_v(r, g, b);
        }
    }
}

static void rgb_to_yuv444p(const image_t *rgb, image_t *yuv) {
    uint32_t size = rgb->width * rgb->height;
    for (uint32_t i = 0; i < size; i++) {
        uint8_t r = rgb->planes.rgb.r[i], g = rgb->planes.rgb.g[i], b = rgb->planes.rgb.b[i];
        yuv->planes.yuv.y[i] = calc_y(r, g, b);
        yuv->planes.yuv.u[i] = calc_u(r, g, b);
        yuv->planes.yuv.v[i] = calc_v(r, g, b);
    }
}

// Public API implementation
int rgb_to_yuv(const image_t *rgb_image,
               image_t *yuv_image,
               yuv_format_t format) {
    // Input validation
    if (!rgb_image || rgb_image->type != IMAGE_TYPE_RGB ||
        !yuv_image || yuv_image->type != IMAGE_TYPE_YUV) {
        return -1;
    }

    // Check dimension consistency
    if (rgb_image->width != yuv_image->width ||
        rgb_image->height != yuv_image->height) {
        return -2;
    }

    // Check format compatibility
    if (yuv_image->planes.yuv.format != format) {
        return -4;
    }

    // Select conversion function based on target format
    switch (format) {
        case YUV_420P:
            rgb_to_yuv420p(rgb_image, yuv_image);
            break;
        case YUV_422P:
            rgb_to_yuv422p(rgb_image, yuv_image);
            break;
        case YUV_444P:
            rgb_to_yuv444p(rgb_image, yuv_image);
            break;
        default:
            return -4;  // Unsupported YUV format
    }

    return 0;
}