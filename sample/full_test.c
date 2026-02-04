#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/image_processor.h"

// Generate simple test Bayer data
int generate_test_data(image_t *raw, bayer_pattern_t pattern) {
    if (!raw || raw->type != IMAGE_TYPE_RAW || !raw->planes.raw.data) return -1;

    uint32_t width = raw->width;
    uint32_t height = raw->height;

    // Generate gradient test pattern
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            uint8_t val = (uint8_t)((x + y * 2) % 256);
            raw->planes.raw.data[idx] = val;
        }
    }

    return 0;
}

// Save YUV file
int save_yuv(const char *filename, const image_t *yuv) {
    if (!filename || !yuv || yuv->type != IMAGE_TYPE_YUV) return -1;

    FILE *fp = fopen(filename, "wb");
    if (!fp) return -2;

    size_t y_size = yuv->width * yuv->height;
    fwrite(yuv->planes.yuv.y, 1, y_size, fp);

    size_t uv_size;
    switch (yuv->planes.yuv.format) {
        case YUV_420P:
            uv_size = (yuv->width / 2) * (yuv->height / 2);
            break;
        case YUV_422P:
            uv_size = (yuv->width / 2) * yuv->height;
            break;
        case YUV_444P:
            uv_size = y_size;
            break;
        default:
            fclose(fp);
            return -3;
    }

    fwrite(yuv->planes.yuv.u, 1, uv_size, fp);
    fwrite(yuv->planes.yuv.v, 1, uv_size, fp);

    fclose(fp);
    return 0;
}

int main() {
    printf("=== Complete RGB to YUV Conversion and Demosaicing Test ===\n\n");

    // Initialization
    printf("1. Initializing image processor...\n");
    if (image_processor_init() != 0) {
        printf("Initialization failed\n");
        return 1;
    }

    // Test different sizes and formats
    struct {
        uint32_t width, height;
        bayer_pattern_t bayer;
        yuv_format_t yuv_fmt;
        const char *name;
    } tests[] = {
        {64, 48, BAYER_RGGB, YUV_420P, "64x48_RGGB_420P"},
        {32, 24, BAYER_GRBG, YUV_422P, "32x24_GRBG_422P"},
        {16, 16, BAYER_BGGR, YUV_444P, "16x16_BGGR_444P"}
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    for (int i = 0; i < test_count; i++) {
        printf("\n2.%d Test: %s\n", i+1, tests[i].name);

        // Create images
        image_t *raw = create_image(tests[i].width, tests[i].height, IMAGE_TYPE_RAW, 8);
        image_t *rgb = create_image(tests[i].width, tests[i].height, IMAGE_TYPE_RGB, 0);
        image_t *yuv = create_image(tests[i].width, tests[i].height, IMAGE_TYPE_YUV, tests[i].yuv_fmt);

        if (!raw || !rgb || !yuv) {
            printf("   Memory allocation failed\n");
            if (raw) free_image(raw);
            if (rgb) free_image(rgb);
            if (yuv) free_image(yuv);
            continue;
        }

        // Generate test data
        if (generate_test_data(raw, tests[i].bayer) != 0) {
            printf("   Test data generation failed\n");
            goto cleanup_test;
        }

        // Demosaicing
        clock_t start = clock();
        int result = demosaic_bayer(raw, tests[i].bayer, rgb);
        clock_t mid = clock();

        if (result != 0) {
            printf("   Demosaicing failed, error code: %d\n", result);
            goto cleanup_test;
        }

        // RGB to YUV conversion
        result = rgb_to_yuv(rgb, yuv, tests[i].yuv_fmt);
        clock_t end = clock();

        if (result != 0) {
            printf("   YUV conversion failed, error code: %d\n", result);
            goto cleanup_test;
        }

        // Calculate processing time
        double demosaic_time = (double)(mid - start) / CLOCKS_PER_SEC * 1000;
        double yuv_time = (double)(end - mid) / CLOCKS_PER_SEC * 1000;

        printf("   Demosaicing: %.2f ms, YUV conversion: %.2f ms\n", demosaic_time, yuv_time);

        // Save result to out directory
        char filename[256];
        snprintf(filename, sizeof(filename), "./out/full_test/output_%s.yuv", tests[i].name);
        if (save_yuv(filename, yuv) == 0) {
            printf("   Saved: out/full_test/output_%s.yuv\n", tests[i].name);
        } else {
            printf("   Save failed: %s\n", filename);
        }

        // Check result data
        printf("   First pixel Y=%d, U=%d, V=%d\n",
               yuv->planes.yuv.y[0], yuv->planes.yuv.u[0], yuv->planes.yuv.v[0]);

        printf("   Test passed ✓\n");
        passed++;

cleanup_test:
        free_image(yuv);
        free_image(rgb);
        free_image(raw);
    }

    // Boundary testing
    printf("\n3. Boundary case testing\n");
    int boundary_passed = 0;

    // Null pointer test
    if (demosaic_bayer(NULL, BAYER_RGGB, NULL) != 0) {
        printf("   Null pointer test passed ✓\n");
        boundary_passed++;
    }

    // Dimension mismatch test
    image_t *raw1 = create_image(10, 10, IMAGE_TYPE_RAW, 8);
    image_t *rgb1 = create_image(20, 20, IMAGE_TYPE_RGB, 0);
    if (raw1 && rgb1) {
        if (demosaic_bayer(raw1, BAYER_RGGB, rgb1) != 0) {
            printf("   Dimension mismatch test passed ✓\n");
            boundary_passed++;
        }
    }
    if (raw1) free_image(raw1);
    if (rgb1) free_image(rgb1);

    // Minimum size test
    image_t *raw2 = create_image(2, 2, IMAGE_TYPE_RAW, 8);
    image_t *rgb2 = create_image(2, 2, IMAGE_TYPE_RGB, 0);
    image_t *yuv2 = create_image(2, 2, IMAGE_TYPE_YUV, YUV_420P);

    if (raw2 && rgb2 && yuv2) {
        generate_test_data(raw2, BAYER_RGGB);
        if (demosaic_bayer(raw2, BAYER_RGGB, rgb2) == 0 &&
            rgb_to_yuv(rgb2, yuv2, YUV_420P) == 0) {
            printf("   Minimum size test passed ✓\n");
            boundary_passed++;
        }
    }
    if (raw2) free_image(raw2);
    if (rgb2) free_image(rgb2);
    if (yuv2) free_image(yuv2);

    // Summary
    printf("\n=== Test Results ===\n");
    printf("Functionality tests: %d/%d passed\n", passed, test_count);
    printf("Boundary tests: %d/3 passed\n", boundary_passed);
    printf("Overall success rate: %.1f%%\n",
           (float)(passed + boundary_passed) / (test_count + 3) * 100);

    // Cleanup
    image_processor_cleanup();
    printf("\nTesting completed!\n");

    return (passed == test_count && boundary_passed >= 2) ? 0 : 1;
}