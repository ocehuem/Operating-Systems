#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "../libppm.h"
#include <cstring>
#include <vector>
#include <cmath>

using namespace std;
using namespace std::chrono;

struct shared_data {
    int width;
    int height;
    vector<float> pixels;  // Change to float for accumulating details
};

atomic<int> stage_flag{0};
const int ITERATIONS = 1000;  // Reduced iterations for more controlled sharpening

// Function to smoothen the image
void S1_smoothen(const image_t *input_image, shared_data &shm_data) {
    shm_data.pixels.resize(input_image->height * input_image->width * 3);

    for (int i = 1; i < input_image->height - 1; ++i) {
        for (int j = 1; j < input_image->width - 1; ++j) {
            for (int k = 0; k <= 2; ++k) {
                float sum = 0;
                for (int a = -1; a <= 1; ++a) {
                    for (int b = -1; b <= 1; ++b) {
                        sum += input_image->image_pixels[i + a][j + b][k];
                    }
                }
                shm_data.pixels[(i * input_image->width + j) * 3 + k] = sum / 9.0f;
            }
        }
    }

    // Handle edges
    for (int i = 0; i < input_image->height; ++i) {
        for (int k = 0; k <= 2; ++k) {
            shm_data.pixels[(i * input_image->width + 0) * 3 + k] = input_image->image_pixels[i][0][k];
            shm_data.pixels[(i * input_image->width + input_image->width - 1) * 3 + k] = input_image->image_pixels[i][input_image->width - 1][k];
        }
    }

    for (int j = 0; j < input_image->width; ++j) {
        for (int k = 0; k <= 2; ++k) {
            shm_data.pixels[(0 * input_image->width + j) * 3 + k] = input_image->image_pixels[0][j][k];
            shm_data.pixels[((input_image->height - 1) * input_image->width + j) * 3 + k] = input_image->image_pixels[input_image->height - 1][j][k];
        }
    }
}

// Function to find details in the image
void S2_find_details(const image_t *input_image, shared_data &shm_data) {
    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k <= 2; ++k) {
                float smoothened_pixel = shm_data.pixels[(i * input_image->width + j) * 3 + k];
                float detail = input_image->image_pixels[i][j][k] - smoothened_pixel;
                shm_data.pixels[(i * input_image->width + j) * 3 + k] = detail;
            }
        }
    }
}

// Function to sharpen the image
void S3_sharpen(const image_t *input_image, shared_data &shm_data, image_t *output_image) {
    static int iteration = 0;
    float sharpening_factor = 1.5f;  // Adjust this to control sharpening intensity

    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k <= 2; ++k) {
                float detail = shm_data.pixels[(i * input_image->width + j) * 3 + k];
                float sharpened = input_image->image_pixels[i][j][k] + detail * sharpening_factor;
                
                // Accumulate sharpening effect over iterations
                if (iteration == 0) {
                    output_image->image_pixels[i][j][k] = static_cast<unsigned char>(round(sharpened));
                } else {
                    float current = output_image->image_pixels[i][j][k];
                    output_image->image_pixels[i][j][k] = static_cast<unsigned char>(round(current + (sharpened - current) / (iteration + 1)));
                }
            }
        }
    }
    iteration++;
}

void thread_S1(const image_t *input_image, shared_data &shm_data) {
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        while (stage_flag != 0) {
            this_thread::yield();
        }
        S1_smoothen(input_image, shm_data);
        stage_flag.store(1, memory_order_release);
    }
}

void thread_S2(const image_t *input_image, shared_data &shm_data) {
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        while (stage_flag != 1) {
            this_thread::yield();
        }
        S2_find_details(input_image, shm_data);
        stage_flag.store(2, memory_order_release);
    }
}

void thread_S3(const image_t *input_image, shared_data &shm_data, image_t *output_image) {
    //auto start = high_resolution_clock::now();

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        while (stage_flag != 2) {
            this_thread::yield();
        }
        S3_sharpen(input_image, shm_data, output_image);
        stage_flag.store(0, memory_order_release);
    }

    /*auto stop = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(stop - start);
    cout << "Total processing time: " << duration.count() << "seconds\n";*/
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        return 1;
    }

    // Start timing the overall process
    high_resolution_clock::time_point start_time = high_resolution_clock::now();

    // Read the input image
    image_t *input_image = read_ppm_file(argv[1]);
    if (!input_image) {
        cerr << "Failed to read input image." << endl;
        return 1;
    }

    shared_data shm_data;
    shm_data.width = input_image->width;
    shm_data.height = input_image->height;

    // Create output image
    image_t *output_image = new image_t;
    output_image->width = input_image->width;
    output_image->height = input_image->height;
    output_image->image_pixels = new unsigned char**[output_image->height];
    for (int i = 0; i < output_image->height; ++i) {
        output_image->image_pixels[i] = new unsigned char*[output_image->width];
        for (int j = 0; j < output_image->width; ++j) {
            output_image->image_pixels[i][j] = new unsigned char[3];
        }
    }

    // Create threads
    thread t1(thread_S1, input_image, ref(shm_data));
    thread t2(thread_S2, input_image, ref(shm_data));
    thread t3(thread_S3, input_image, ref(shm_data), output_image);

    // Wait for threads to finish
    t1.join();
    t2.join();
    t3.join();

    // Write output image
    write_ppm_file(argv[2], output_image);

    // Clean up
    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            delete[] input_image->image_pixels[i][j];
        }
        delete[] input_image->image_pixels[i];
    }
    delete input_image;

    for (int i = 0; i < output_image->height; ++i) {
        for (int j = 0; j < output_image->width; ++j) {
            delete[] output_image->image_pixels[i][j];
        }
        delete[] output_image->image_pixels[i];
    }
    delete output_image;

    // End timing the overall process
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    microseconds total_duration = duration_cast<microseconds>(end_time - start_time);

    // Convert microseconds to seconds (1 second = 1,000,000 microseconds)
    double total_duration_sec = total_duration.count() / 1000000.0;

    // Print the timings in seconds
    cout << "Total time: " << total_duration_sec << " seconds" << endl;

    return 0;
}
