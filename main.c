#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void grayscale_averaging_c(unsigned char* pixels, int width, int height);
void grayscale_averaging_asm(unsigned char* pixels, int width, int height);

double time_diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.png> <output.png>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    int width, height, channels_in_file;
    unsigned char* img = stbi_load(input_file, &width, &height, &channels_in_file, 3);
    if (!img) {
        fprintf(stderr, "Error: cannot load %s\n", input_file);
        fprintf(stderr, "Reason: %s\n", stbi_failure_reason());
        return 1;
    }

    printf("Loaded %dx%d, original channels: %d, forced RGB\n", width, height, channels_in_file);

    unsigned char* work_c = malloc(width * height * 3);
    memcpy(work_c, img, width * height * 3);
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    grayscale_averaging_c(work_c, width, height);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    double time_c = time_diff_ms(start, end);
    char out_c[256];
    snprintf(out_c, sizeof(out_c), "c_%s", output_file);
    stbi_write_png(out_c, width, height, 3, work_c, width * 3);
    printf("C version: %.3f ms, saved to %s\n", time_c, out_c);
    free(work_c);

    unsigned char* work_asm = malloc(width * height * 3);
    memcpy(work_asm, img, width * height * 3);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    grayscale_averaging_asm(work_asm, width, height);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    double time_asm = time_diff_ms(start, end);
    char out_asm[256];
    snprintf(out_asm, sizeof(out_asm), "asm_%s", output_file);
    stbi_write_png(out_asm, width, height, 3, work_asm, width * 3);
    printf("ASM version: %.3f ms, saved to %s\n", time_asm, out_asm);
    free(work_asm);

    stbi_image_free(img);
    return 0;
}
