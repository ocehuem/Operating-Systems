#include <iostream>
#include <chrono>
#include <unistd.h>
#include "../libppm.h"
#include <sys/wait.h>
#include <cstring>
#include <vector>

using namespace std;
using namespace std::chrono;

// Function to serialize image data into a byte buffer
vector<unsigned char> serialize_image(struct image_t* img) {
    vector<unsigned char> buffer;
    // Serialize height and width
    buffer.resize(2 * sizeof(int));
    memcpy(buffer.data(), &(img->height), sizeof(int));
    memcpy(buffer.data() + sizeof(int), &(img->width), sizeof(int));
    // Serialize pixel data
    for(int i = 0; i < img->height; ++i) {
        for(int j = 0; j < img->width; ++j) {
            buffer.insert(buffer.end(), img->image_pixels[i][j], img->image_pixels[i][j] + 3);
        }
    }
    return buffer;
}

// Function to deserialize image data from a byte buffer
struct image_t* deserialize_image(const unsigned char* data, size_t size) {
    if(size < 2 * sizeof(int)) {
        cerr << "Invalid data size for image deserialization." << endl;
        return nullptr;
    }
    struct image_t* img = new struct image_t;
    memcpy(&(img->height), data, sizeof(int));
    memcpy(&(img->width), data + sizeof(int), sizeof(int));
    int m = img->height;
    int n = img->width;
    img->image_pixels = new unsigned char**[m];
    int offset = 2 * sizeof(int);
    for(int i = 0; i < m; i++) {
        img->image_pixels[i] = new unsigned char*[n];
        for(int j = 0; j < n; j++) {
            img->image_pixels[i][j] = new unsigned char[3];
            memcpy(img->image_pixels[i][j], data + offset, 3 * sizeof(unsigned char));
            offset += 3 * sizeof(unsigned char);
        }
    }
    return img;
}

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

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        return 1;
    }
  
    // Start timing the overall process
    high_resolution_clock::time_point start_time = high_resolution_clock::now();

    // Create pipes
    int pipefd1[2], pipefd2[2];
    if(pipe(pipefd1) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if(pipe(pipefd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork S1 process
    pid_t pid1 = fork();
    if(pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid1 == 0) {  // Child 1 (S1)
        close(pipefd1[0]);  // Close read end of pipe1
        // Read the input image once
        struct image_t *input_image = read_ppm_file(argv[1]);
        if(!input_image) {
            cerr << "Failed to read input image in S1." << endl;
            exit(EXIT_FAILURE);
        }

    // Start time measurement
    //auto start = high_resolution_clock::now();

        for(int iter = 0; iter < 1000; ++iter) {
            struct image_t *smoothened_image = S1_smoothen(input_image);
            // Serialize the image
            vector<unsigned char> buffer = serialize_image(smoothened_image);
            // Write the size of the buffer first
            int size = buffer.size();
            if(write(pipefd1[1], &size, sizeof(int)) != sizeof(int)) {
                perror("write size to pipe1");
                exit(EXIT_FAILURE);
            }
            // Write the buffer
            if(write(pipefd1[1], buffer.data(), size) != size) {
                perror("write buffer to pipe1");
                exit(EXIT_FAILURE);
            }
            // Clean up
            // Free smoothened_image
            for(int i = 0; i < smoothened_image->height; i++) {
                for(int j = 0; j < smoothened_image->width; j++) {
                    delete[] smoothened_image->image_pixels[i][j];
                }
                delete[] smoothened_image->image_pixels[i];
            }
            delete smoothened_image;
        }

        // Close write end after sending all data
        close(pipefd1[1]);
        // Clean up input_image
        for(int i = 0; i < input_image->height; i++) {
            for(int j = 0; j < input_image->width; j++) {
                delete[] input_image->image_pixels[i][j];
            }
            delete[] input_image->image_pixels[i];
        }
        delete input_image;
        exit(0);
    }

    // Fork S2 process
    pid_t pid2 = fork();
    if(pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid2 == 0) {  // Child 2 (S2)
        close(pipefd1[1]);  // Close write end of pipe1
        close(pipefd2[0]);  // Close read end of pipe2

        for(int iter = 0; iter < 1000; ++iter) {
            // Read the size first
            int size;
            ssize_t bytes_read = read(pipefd1[0], &size, sizeof(int));
            if(bytes_read == 0) {  // EOF
                break;
            }
            if(bytes_read != sizeof(int)) {
                perror("read size from pipe1");
                exit(EXIT_FAILURE);
            }
            // Read the buffer
            vector<unsigned char> buffer(size);
            ssize_t total_read = 0;
            while(total_read < size) {
                ssize_t r = read(pipefd1[0], buffer.data() + total_read, size - total_read);
                if(r <= 0) {
                    perror("read buffer from pipe1");
                    exit(EXIT_FAILURE);
                }
                total_read += r;
            }
            // Deserialize the image
            struct image_t* smoothened_image = deserialize_image(buffer.data(), size);
            if(!smoothened_image) {
                cerr << "Failed to deserialize image in S2." << endl;
                exit(EXIT_FAILURE);
            }
            // Read the original image once (since input_image is same for all iterations)
            static struct image_t* input_image = nullptr;
            if(!input_image) {
                input_image = read_ppm_file(argv[1]);
                if(!input_image) {
                    cerr << "Failed to read input image in S2." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            // Perform S2
            struct image_t *details_image = S2_find_details(input_image, smoothened_image);
            // Serialize the details_image
            vector<unsigned char> out_buffer = serialize_image(details_image);
            int out_size = out_buffer.size();
            // Write the size first
            if(write(pipefd2[1], &out_size, sizeof(int)) != sizeof(int)) {
                perror("write size to pipe2");
                exit(EXIT_FAILURE);
            }
            // Write the buffer
            if(write(pipefd2[1], out_buffer.data(), out_size) != out_size) {
                perror("write buffer to pipe2");
                exit(EXIT_FAILURE);
            }
            // Clean up
            // Free smoothened_image and details_image
            for(int i = 0; i < smoothened_image->height; i++) {
                for(int j = 0; j < smoothened_image->width; j++) {
                    delete[] smoothened_image->image_pixels[i][j];
                }
                delete[] smoothened_image->image_pixels[i];
            }
            delete smoothened_image;

            for(int i = 0; i < details_image->height; i++) {
                for(int j = 0; j < details_image->width; j++) {
                    delete[] details_image->image_pixels[i][j];
                }
                delete[] details_image->image_pixels[i];
            }
            delete details_image;
        }

        // Close pipes
        close(pipefd1[0]);
        close(pipefd2[1]);

        // Clean up input_image
        if(pid2 == 0) {
            // Only delete input_image once
            // Since it's static, delete after loop ends
            // However, since the process is exiting, it's optional
        }
        exit(0);
    }

    // Fork S3 process
    pid_t pid3 = fork();
    if(pid3 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid3 == 0) {  // Child 3 (S3)
        close(pipefd1[0]);  // Close both ends of pipe1
        close(pipefd1[1]);
        close(pipefd2[1]);  // Close write end of pipe2

        struct image_t* final_sharpened_image = nullptr;

        // Start time measurement
        auto start = high_resolution_clock::now();


        for(int iter = 0; iter < 1000; ++iter) {
            // Read the size first
            int size;
            ssize_t bytes_read = read(pipefd2[0], &size, sizeof(int));
            if(bytes_read == 0) {  // EOF
                break;
            }
            if(bytes_read != sizeof(int)) {
                perror("read size from pipe2");
                exit(EXIT_FAILURE);
            }
            // Read the buffer
            vector<unsigned char> buffer(size);
            ssize_t total_read = 0;
            while(total_read < size) {
                ssize_t r = read(pipefd2[0], buffer.data() + total_read, size - total_read);
                if(r <= 0) {
                    perror("read buffer from pipe2");
                    exit(EXIT_FAILURE);
                }
                total_read += r;
            }
            // Deserialize the details_image
            struct image_t* details_image = deserialize_image(buffer.data(), size);
            if(!details_image) {
                cerr << "Failed to deserialize image in S3." << endl;
                exit(EXIT_FAILURE);
            }
            // Read the original image once (since input_image is same for all iterations)
            static struct image_t* input_image = nullptr;
            if(!input_image) {
                input_image = read_ppm_file(argv[1]);
                if(!input_image) {
                    cerr << "Failed to read input image in S3." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            // Perform S3
            struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
            // Keep the last sharpened image
            if(final_sharpened_image) {
                // Free the previous image
                for(int i = 0; i < final_sharpened_image->height; i++) {
                    for(int j = 0; j < final_sharpened_image->width; j++) {
                        delete[] final_sharpened_image->image_pixels[i][j];
                    }
                    delete[] final_sharpened_image->image_pixels[i];
                }
                delete final_sharpened_image;
            }
            final_sharpened_image = sharpened_image;
            
  

            // Clean up
            // Free details_image
            for(int i = 0; i < details_image->height; i++) {
                for(int j = 0; j < details_image->width; j++) {
                    delete[] details_image->image_pixels[i][j];
                }
                delete[] details_image->image_pixels[i];
            }
            delete details_image;
        }


          // Stop time measurement
    auto stop = high_resolution_clock::now();

    // Measure duration
    auto duration = duration_cast<seconds>(stop - start);

    // Print the total processing time
    cout << "Total processing time (excluding file I/O): " << duration.count() << "seconds\n";


        // Write the final sharpened image to file
        if(final_sharpened_image) {
            write_ppm_file(argv[2], final_sharpened_image);
            // Free the final image
            for(int i = 0; i < final_sharpened_image->height; i++) {
                for(int j = 0; j < final_sharpened_image->width; j++) {
                    delete[] final_sharpened_image->image_pixels[i][j];
                }
                delete[] final_sharpened_image->image_pixels[i];
            }
            delete final_sharpened_image;
        }

        // Close read end of pipe2
        close(pipefd2[0]);

        // Clean up input_image
        if(pid3 == 0) {
            // Only delete input_image once
            // Since it's static, delete after loop ends
            // However, since the process is exiting, it's optional
        }
        exit(0);
    }

    // Parent process closes all pipe ends
    close(pipefd1[0]);
    close(pipefd1[1]);
    close(pipefd2[0]);
    close(pipefd2[1]);

    // Wait for all child processes to finish
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // End timing the overall process
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    microseconds total_duration = duration_cast<microseconds>(end_time - start_time);

    // Convert microseconds to seconds (1 second = 1,000,000 microseconds)
    double total_duration_sec = total_duration.count() / 1000000.0;

    // Print the timings in seconds
    cout << "Total time: " << total_duration_sec << " seconds" << endl;

    return 0;
}
