#include <iostream>
#include "../libppm.h"
#include <cstdint>
#include <chrono>


using namespace std;
using namespace std::chrono;

// Helper function to allocate memory for image pixels
uint8_t*** allocate_image_pixels(int width, int height) {
    uint8_t*** pixels = new uint8_t**[height];
    for (int i = 0; i < height; ++i) {
        pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; ++j) {
            pixels[i][j] = new uint8_t[3];
        }
    }
    return pixels;
}

// Function to smoothen the image
struct image_t* S1_smoothen(struct image_t *input_image) {
    image_t *smoothened_image = new image_t;
    smoothened_image->width = input_image->width;
    smoothened_image->height = input_image->height;
    smoothened_image->image_pixels = allocate_image_pixels(input_image->width, input_image->height);

    for (int i = 1; i < input_image->height - 1; ++i) {
        for (int j = 1; j < input_image->width - 1; ++j) {
            for (int k = 0; k <= 2; ++k) {
                int sum = 0;
                for (int a = -1; a <= 1; ++a) {
                    for (int b = -1; b <= 1; ++b) {
                        sum += input_image->image_pixels[i + a][j + b][k];
                    }
                }
                uint8_t average = static_cast<uint8_t>(sum / 9);
                smoothened_image->image_pixels[i][j][k] = average;
            }
        }
    }

    // Handle edges
    for (int i = 0; i < input_image->height; ++i) {
        for (int k = 0; k <= 2; ++k) {
            smoothened_image->image_pixels[i][0][k] = input_image->image_pixels[i][0][k];
            smoothened_image->image_pixels[i][input_image->width - 1][k] = input_image->image_pixels[i][input_image->width - 1][k];
        }
    }

    for (int j = 0; j < input_image->width; ++j) {
        for (int k = 0; k <= 2; ++k) {
            smoothened_image->image_pixels[0][j][k] = input_image->image_pixels[0][j][k];
            smoothened_image->image_pixels[input_image->height - 1][j][k] = input_image->image_pixels[input_image->height - 1][j][k];
        }
    }

    return smoothened_image;
}

// Function to find details in the image
struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image) {
    image_t *details_image = new image_t;
    details_image->width = input_image->width;
    details_image->height = input_image->height;
    details_image->image_pixels = allocate_image_pixels(input_image->width, input_image->height);

    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k <= 2; ++k) {
                details_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k];
            }
        }
    }

    return details_image;
}

// Function to sharpen the image
struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image) {
    image_t *sharpened_image = new image_t;
    sharpened_image->width = input_image->width;
    sharpened_image->height = input_image->height;
    sharpened_image->image_pixels = allocate_image_pixels(input_image->width, input_image->height);

    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k <= 2; ++k) {
                sharpened_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] + details_image->image_pixels[i][j][k];
            }
        }
    }

    return sharpened_image;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }

    // Start timing the overall process
    high_resolution_clock::time_point start_time = high_resolution_clock::now();
    
    struct image_t *input_image = read_ppm_file(argv[1]);
    struct image_t *smoothened_image = S1_smoothen(input_image);
    struct image_t *details_image = S2_find_details(input_image, smoothened_image);
    struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
    
    for(int i=0; i<1000; ++i){
      struct image_t *smoothened_image = S1_smoothen(input_image);
      struct image_t *details_image = S2_find_details(input_image, smoothened_image);
      struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
    }
    
    write_ppm_file(argv[2], sharpened_image);
    
    // End timing the overall process
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    microseconds total_duration = duration_cast<microseconds>(end_time - start_time);

    // Convert microseconds to seconds (1 second = 1,000,000 microseconds)
    double total_duration_sec = total_duration.count() / 1000000.0;

    // Print the timings in seconds
    cout << "Total time: " << total_duration_sec << " seconds" << endl;
    

    return 0;
}

