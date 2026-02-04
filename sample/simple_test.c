#include <stdio.h>
#include <stdlib.h>
#include "../include/image_processor.h"

int main() {
    printf("Starting simple test...\n");

    // Test initialization
    printf("1. Initializing image processor...\n");
    if (image_processor_init() != 0) {
        printf("Initialization failed\n");
        return 1;
    }
    printf("Initialization successful\n");

    // Test creating image data structures
    printf("2. Creating image data structures...\n");
    image_t *raw = create_image(4, 4, IMAGE_TYPE_RAW, 8);
    if (!raw) {
        printf("Failed to create RAW image\n");
        return 1;
    }
    printf("Created RAW image successfully: %dx%d\n", raw->width, raw->height);

    image_t *rgb = create_image(4, 4, IMAGE_TYPE_RGB, 0);
    if (!rgb) {
        printf("Failed to create RGB image\n");
        free_image(raw);
        return 1;
    }
    printf("Created RGB image successfully: %dx%d\n", rgb->width, rgb->height);

    // Simple test data
    printf("3. Filling test data...\n");
    for (int i = 0; i < 16; i++) {
        raw->planes.raw.data[i] = i * 16;  // 0, 16, 32, 48, ...
    }
    printf("Test data filling completed\n");

    // Test demosaicing
    printf("4. Performing demosaicing...\n");
    int result = demosaic_bayer(raw, BAYER_RGGB, rgb);
    if (result != 0) {
        printf("Demosaicing failed, error code: %d\n", result);
        free_image(rgb);
        free_image(raw);
        return 1;
    }
    printf("Demosaicing successful\n");

    // Output partial results
    printf("5. Checking results...\n");
    printf("First pixel RGB: (%d, %d, %d)\n",
           rgb->planes.rgb.r[0], rgb->planes.rgb.g[0], rgb->planes.rgb.b[0]);

    // Cleanup
    printf("6. Cleaning up resources...\n");
    free_image(rgb);
    free_image(raw);
    image_processor_cleanup();

    printf("Simple test completed successfully!\n");
    return 0;
}