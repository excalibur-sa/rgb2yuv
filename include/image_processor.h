#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <stdint.h>
#include <stddef.h>

// Bayer pattern definitions
typedef enum {
    BAYER_RGGB = 0,  // Red-Green-Green-Blue
    BAYER_GRBG = 1,  // Green-Red-Blue-Green
    BAYER_GBRG = 2,  // Green-Blue-Red-Green
    BAYER_BGGR = 3   // Blue-Green-Green-Red
} bayer_pattern_t;

// YUV format definitions
typedef enum {
    YUV_420P = 0,    // YUV420 Planar
    YUV_422P = 1,    // YUV422 Planar
    YUV_444P = 2     // YUV444 Planar
} yuv_format_t;

// Image type definitions
typedef enum {
    IMAGE_TYPE_RAW,
    IMAGE_TYPE_RGB,
    IMAGE_TYPE_YUV
} image_type_t;

// Unified image data structure
typedef struct {
    image_type_t type;
    uint32_t width;
    uint32_t height;
    union {
        struct {
            uint8_t *data;
            uint32_t stride;
            uint8_t bpp;
        } raw;
        struct {
            uint8_t *r;
            uint8_t *g;
            uint8_t *b;
        } rgb;
        struct {
            uint8_t *y;
            uint8_t *u;
            uint8_t *v;
            yuv_format_t format;
        } yuv;
    } planes;
} image_t;

// Backward compatibility typedefs
typedef image_t image_data_t;
typedef image_t rgb_image_t;
typedef image_t yuv_image_t;

// Function declarations

/**
 * Initialize image processor
 * @return 0 on success, non-zero on failure
 */
int image_processor_init(void);

/**
 * Cleanup image processor resources
 */
void image_processor_cleanup(void);

/**
 * Bayer pattern RAW image demosaicing
 * @param raw_image Input RAW image data
 * @param pattern Bayer pattern
 * @param rgb_image Output RGB image data
 * @return 0 on success, non-zero on failure
 */
int demosaic_bayer(const image_t *raw_image,
                   bayer_pattern_t pattern,
                   image_t *rgb_image);

/**
 * Convert RGB image to YUV format
 * @param rgb_image Input RGB image
 * @param yuv_image Output YUV image
 * @param format Target YUV format
 * @return 0 on success, non-zero on failure
 */
int rgb_to_yuv(const image_t *rgb_image,
               image_t *yuv_image,
               yuv_format_t format);

/**
 * Unified image creation interface
 * @param width Width
 * @param height Height
 * @param type Image type
 * @param format_or_bpp For RAW it's bpp, for YUV it's yuv_format_t
 * @return Image structure pointer, NULL on failure
 */
image_t* create_image(uint32_t width, uint32_t height, image_type_t type, uint32_t format_or_bpp);

/**
 * Unified image release interface
 * @param image Image pointer
 */
void free_image(image_t *image);

#endif // IMAGE_PROCESSOR_H