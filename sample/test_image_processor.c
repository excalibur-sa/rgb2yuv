#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/image_processor.h"

// Test case structure
typedef struct {
    const char *name;
    uint32_t width;
    uint32_t height;
    bayer_pattern_t pattern;
    yuv_format_t yuv_format;
} test_case_t;

// Function declarations
static int generate_test_bayer_data(image_t *image, bayer_pattern_t pattern);
static int save_yuv_file(const char *filename, const image_t *yuv);
static void print_test_header(const char *test_name);
static void print_test_result(const char *test_name, int result);
static int test_basic_functionality(void);
static int test_different_formats(void);
static int test_edge_cases(void);
static void print_image_info(const char *prefix, uint32_t width, uint32_t height);

// Test cases array
static test_case_t test_cases[] = {
    {"Basic RGGB to YUV420P", 640, 480, BAYER_RGGB, YUV_420P},
    {"GRBG to YUV422P", 320, 240, BAYER_GRBG, YUV_422P},
    {"GBRG to YUV444P", 160, 120, BAYER_GBRG, YUV_444P},
    {"BGGR to YUV420P", 800, 600, BAYER_BGGR, YUV_420P},
};

int main(int argc, char *argv[]) {
    printf("=== RGB to YUV Conversion and Demosaicing Test Program ===\n");

    // Initialize image processor
    if (image_processor_init() != 0) {
        printf("Error: Failed to initialize image processor\n");
        return 1;
    }

    int total_tests = 0;
    int passed_tests = 0;

    // Basic functionality test
    print_test_header("Basic Functionality Test");
    if (test_basic_functionality() == 0) {
        passed_tests++;
        print_test_result("Basic Functionality Test", 0);
    } else {
        print_test_result("Basic Functionality Test", -1);
    }
    total_tests++;

    // Different formats test
    print_test_header("Different Formats Test");
    if (test_different_formats() == 0) {
        passed_tests++;
        print_test_result("Different Formats Test", 0);
    } else {
        print_test_result("Different Formats Test", -1);
    }
    total_tests++;

    // Edge cases test
    print_test_header("Edge Cases Test");
    if (test_edge_cases() == 0) {
        passed_tests++;
        print_test_result("Edge Cases Test", 0);
    } else {
        print_test_result("Edge Cases Test", -1);
    }
    total_tests++;

    // Print test results summary
    printf("\n=== Test Results Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed tests: %d\n", passed_tests);
    printf("Failed tests: %d\n", total_tests - passed_tests);
    printf("Success rate: %.1f%%\n", (float)passed_tests / total_tests * 100);

    // Cleanup
    image_processor_cleanup();

    return (passed_tests == total_tests) ? 0 : 1;
}

static int test_basic_functionality(void) {
    printf("  Test case: Basic RGGB to YUV420P functionality\n");

    uint32_t width = 320;
    uint32_t height = 240;
    int result = 0;

    // Create test data
    image_t *raw_image = create_image(width, height, IMAGE_TYPE_RAW, 8);
    if (!raw_image) {
        printf("    Error: Failed to create RAW image\n");
        return -1;
    }
    
    image_t *rgb_image = create_image(width, height, IMAGE_TYPE_RGB, 0);
    if (!rgb_image) {
        printf("    Error: Failed to create RGB image\n");
        free_image(raw_image);
        return -1;
    }
    
    image_t *yuv_image = create_image(width, height, IMAGE_TYPE_YUV, YUV_420P);
    if (!yuv_image) {
        printf("    Error: Failed to create YUV image\n");
        free_image(rgb_image);
        free_image(raw_image);
        return -1;
    }
    
    // Generate test Bayer data
    if (generate_test_bayer_data(raw_image, BAYER_RGGB) != 0) {
        printf("    Error: Failed to generate test data\n");
        result = -1;
        goto cleanup;
    }

    print_image_info("  RAW image", width, height);
    printf("\n");

    // Perform demosaicing
    clock_t start_time = clock();
    result = demosaic_bayer(raw_image, BAYER_RGGB, rgb_image);
    clock_t end_time = clock();

    if (result != 0) {
        printf("    Error: Demosaicing failed, error code: %d\n", result);
        goto cleanup;
    }

    double demosaic_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000;
    printf("  Demosaicing processing time: %.2f ms\n", demosaic_time);

    // Perform RGB to YUV conversion
    start_time = clock();
    result = rgb_to_yuv(rgb_image, yuv_image, YUV_420P);
    end_time = clock();

    if (result != 0) {
        printf("    Error: RGB to YUV conversion failed, error code: %d\n", result);
        goto cleanup;
    }

    double yuv_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000;
    printf("  YUV conversion processing time: %.2f ms\n", yuv_time);

    // Save result files to out directory
    char filename[256];
    snprintf(filename, sizeof(filename), "./out/test_image_processor/test_output_basic_%dx%d.yuv", width, height);
    if (save_yuv_file(filename, yuv_image) == 0) {
        printf("  Output file saved: out/test_image_processor/test_output_basic_%dx%d.yuv\n", width, height);
    } else {
        printf("  Failed to save output file: %s\n", filename);
    }

    printf("  Basic functionality test passed\n");
    result = 0;

cleanup:
    free_image(yuv_image);
    free_image(rgb_image);
    free_image(raw_image);
    return result;
}

static int test_different_formats(void) {
    printf("  Testing different Bayer patterns and YUV formats\n");

    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    int failed_cases = 0;

    for (int i = 0; i < num_cases; i++) {
        test_case_t *tc = &test_cases[i];
        printf("    Test case %d: %s (%dx%d)\n", i+1, tc->name, tc->width, tc->height);

        // Create image data
        image_t *raw = create_image(tc->width, tc->height, IMAGE_TYPE_RAW, 8);
        image_t *rgb = create_image(tc->width, tc->height, IMAGE_TYPE_RGB, 0);
        image_t *yuv = create_image(tc->width, tc->height, IMAGE_TYPE_YUV, tc->yuv_format);
        
        if (!raw || !rgb || !yuv) {
            printf("      Error: Memory allocation failed\n");
            failed_cases++;
            if (raw) free_image(raw);
            if (rgb) free_image(rgb);
            if (yuv) free_image(yuv);
            continue;
        }
        
        // Generate test data and process
        if (generate_test_bayer_data(raw, tc->pattern) != 0 ||
            demosaic_bayer(raw, tc->pattern, rgb) != 0 ||
            rgb_to_yuv(rgb, yuv, tc->yuv_format) != 0) {
            printf("      Error: Processing failed\n");
            failed_cases++;
        } else {
            printf("      Success\n");

            // Save output file to out directory
            char filename[256];
            snprintf(filename, sizeof(filename), "./out/test_image_processor/test_output_%d_%dx%d.yuv",
                    i, tc->width, tc->height);
            if (save_yuv_file(filename, yuv) == 0) {
                printf("      Output file saved: out/test_image_processor/test_output_%d_%dx%d.yuv\n", i, tc->width, tc->height);
            } else {
                printf("      Failed to save output file: %s\n", filename);
            }
        }

        free_image(yuv);
        free_image(rgb);
        free_image(raw);
    }

    printf("  Format test completed, failed cases: %d/%d\n", failed_cases, num_cases);
    return (failed_cases == 0) ? 0 : -1;
}

static int test_edge_cases(void) {
    printf("  Testing edge cases\n");

    int failed_tests = 0;

    // Test null pointers
    printf("    Testing null pointer handling...\n");
    if (demosaic_bayer(NULL, BAYER_RGGB, NULL) == 0) {
        printf("      Error: Should reject null pointers\n");
        failed_tests++;
    } else {
        printf("      Null pointer handling correct\n");
    }

    // Test dimension mismatch
    printf("    Testing dimension mismatch...\n");
    image_t *raw = create_image(100, 100, IMAGE_TYPE_RAW, 8);
    image_t *rgb = create_image(200, 200, IMAGE_TYPE_RGB, 0);
    
    if (raw && rgb) {
        if (demosaic_bayer(raw, BAYER_RGGB, rgb) == 0) {
            printf("      Error: Should reject dimension mismatch\n");
            failed_tests++;
        } else {
            printf("      Dimension mismatch handling correct\n");
        }
    }
    
    if (raw) free_image(raw);
    if (rgb) free_image(rgb);
    
    // Test minimum dimensions
    printf("    Testing minimum size images...\n");
    raw = create_image(2, 2, IMAGE_TYPE_RAW, 8);
    rgb = create_image(2, 2, IMAGE_TYPE_RGB, 0);
    image_t *yuv = create_image(2, 2, IMAGE_TYPE_YUV, YUV_420P);
    
    if (raw && rgb && yuv) {
        generate_test_bayer_data(raw, BAYER_RGGB);
        if (demosaic_bayer(raw, BAYER_RGGB, rgb) != 0 ||
            rgb_to_yuv(rgb, yuv, YUV_420P) != 0) {
            printf("      Error: Minimum size processing failed\n");
            failed_tests++;
        } else {
            printf("      Minimum size handling correct\n");
        }
    }
    
    if (raw) free_image(raw);
    if (rgb) free_image(rgb);
    if (yuv) free_image(yuv);
    
    printf("  Edge cases test completed, failed tests: %d\n", failed_tests);
    return (failed_tests == 0) ? 0 : -1;
}

static int generate_test_bayer_data(image_t *image, bayer_pattern_t pattern) {
    if (!image || image->type != IMAGE_TYPE_RAW || !image->planes.raw.data) {
        return -1;
    }

    uint32_t width = image->width;
    uint32_t height = image->height;
    uint8_t *data = image->planes.raw.data;

    // Generate gradient test pattern
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;

            // Generate test value based on position
            uint8_t base_val = (uint8_t)((x + y) % 256);

            // Determine color channel based on Bayer pattern and position
            uint8_t channel_val;
            switch (pattern) {
                case BAYER_RGGB:
                    if ((y % 2 == 0) && (x % 2 == 0)) {
                        // R position - red enhanced
                        channel_val = (base_val * 3 + 255) / 4;
                    } else if ((y % 2 == 1) && (x % 2 == 1)) {
                        // B position - blue enhanced
                        channel_val = (base_val * 2 + 128) / 3;
                    } else {
                        // G position - green standard
                        channel_val = base_val;
                    }
                    break;
                case BAYER_GRBG:
                case BAYER_GBRG:
                case BAYER_BGGR:
                default:
                    // Simplified handling, use similar pattern
                    channel_val = base_val;
                    break;
            }

            image->planes.raw.data[idx] = channel_val;
        }
    }

    return 0;
}

static int save_yuv_file(const char *filename, const image_t *yuv) {
    if (!filename || !yuv || yuv->type != IMAGE_TYPE_YUV) {
        return -1;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen failed");
        return -2;
    }

    // Write Y component
    size_t y_size = yuv->width * yuv->height;
    if (fwrite(yuv->planes.yuv.y, 1, y_size, fp) != y_size) {
        fclose(fp);
        return -3;
    }

    // Calculate UV component size
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
            return -4;
    }

    // Write U and V components
    if (fwrite(yuv->planes.yuv.u, 1, uv_size, fp) != uv_size ||
        fwrite(yuv->planes.yuv.v, 1, uv_size, fp) != uv_size) {
        fclose(fp);
        return -5;
    }

    fclose(fp);
    return 0;
}

static void print_test_header(const char *test_name) {
    printf("--- %s ---\n", test_name);
}

static void print_test_result(const char *test_name, int result) {
    if (result == 0) {
        printf("✓ %s: Passed\n", test_name);
    } else {
        printf("✗ %s: Failed (Error code: %d)\n", test_name, result);
    }
}

static void print_image_info(const char *prefix, uint32_t width, uint32_t height) {
    printf("%s dimensions: %dx%d, Total pixels: %d\n", prefix, width, height, width * height);
}