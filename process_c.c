void grayscale_averaging_c(unsigned char* pixels, int width, int height) {
    int total_pixels = width * height;
    for (int i = 0; i < total_pixels; ++i) {
        int offset = i * 3;
        unsigned char r = pixels[offset];
        unsigned char g = pixels[offset + 1];
        unsigned char b = pixels[offset + 2];
        unsigned char gray = (r + g + b) / 3;
        pixels[offset] = gray;
        pixels[offset + 1] = gray;
        pixels[offset + 2] = gray;
    }
}
