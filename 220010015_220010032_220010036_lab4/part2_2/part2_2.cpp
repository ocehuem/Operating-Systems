#include <iostream>
#include <cstdint>
#include <chrono>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include "../libppm.h"
#include <sys/wait.h>

using namespace std;
using namespace std::chrono;

// Function to smoothen the image
void S1_smoothen(struct image_t *input_image, struct image_t *smoothened_image) {
    for (int i = 1; i < input_image->height - 1; ++i) {
        for (int j = 1; j < input_image->width - 1; ++j) {
            for (int k = 0; k < 3; ++k) {
                int sum = 0;
                for (int a = -1; a <= 1; ++a) {
                    for (int b = -1; b <= 1; ++b) {
                        sum += input_image->image_pixels[i + a][j + b][k];
                    }
                }
                smoothened_image->image_pixels[i][j][k] = static_cast<uint8_t>(sum / 9);
            }
        }
    }

    // Handle edges
    for (int i = 0; i < input_image->height; ++i) {
        for (int k = 0; k < 3; ++k) {
            smoothened_image->image_pixels[i][0][k] = input_image->image_pixels[i][0][k];
            smoothened_image->image_pixels[i][input_image->width - 1][k] = input_image->image_pixels[i][input_image->width - 1][k];
        }
    }

    for (int j = 0; j < input_image->width; ++j) {
        for (int k = 0; k < 3; ++k) {
            smoothened_image->image_pixels[0][j][k] = input_image->image_pixels[0][j][k];
            smoothened_image->image_pixels[input_image->height - 1][j][k] = input_image->image_pixels[input_image->height - 1][j][k];
        }
    }
}

// Function to find details in the image
void S2_find_details(struct image_t *input_image, struct image_t *smoothened_image, struct image_t *details_image) {
    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k < 3; ++k) {
                details_image->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k];
            }
        }
    }
}

// Function to sharpen the image
void S3_sharpen(struct image_t *input_image, struct image_t *details_image, struct image_t *sharpened_image) {
    for (int i = 0; i < input_image->height; ++i) {
        for (int j = 0; j < input_image->width; ++j) {
            for (int k = 0; k < 3; ++k) {
                int sharpened_value = input_image->image_pixels[i][j][k] + 0.1 * details_image->image_pixels[i][j][k];
                sharpened_image->image_pixels[i][j][k] = static_cast<uint8_t>(max(0, min(255, sharpened_value)));
            }
        }
    }
}

// Function to create shared memory and semaphores
void create_shared_resources(struct image_t **smoothened_image, struct image_t **details_image, struct image_t **sharpened_image, sem_t **sem_s1_s2, sem_t **sem_s2_s3, const struct image_t *input_image) {
    int shm_fd;
    size_t image_size = sizeof(struct image_t) + 
                        input_image->height * sizeof(uint8_t**) + 
                        input_image->height * input_image->width * sizeof(uint8_t*) + 
                        input_image->height * input_image->width * 3 * sizeof(uint8_t);

    shm_fd = shm_open("/smoothened_image", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open smoothened_image");
        exit(1);
    }
    if (ftruncate(shm_fd, image_size) == -1) {
        perror("ftruncate smoothened_image");
        exit(1);
    }
    *smoothened_image = (struct image_t *)mmap(0, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*smoothened_image == MAP_FAILED) {
        perror("mmap smoothened_image");
        exit(1);
    }
    close(shm_fd);

    shm_fd = shm_open("/details_image", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open details_image");
        exit(1);
    }
    if (ftruncate(shm_fd, image_size) == -1) {
        perror("ftruncate details_image");
        exit(1);
    }
    *details_image = (struct image_t *)mmap(0, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*details_image == MAP_FAILED) {
        perror("mmap details_image");
        exit(1);
    }
    close(shm_fd);

    shm_fd = shm_open("/sharpened_image", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open sharpened_image");
        exit(1);
    }
    if (ftruncate(shm_fd, image_size) == -1) {
        perror("ftruncate sharpened_image");
        exit(1);
    }
    *sharpened_image = (struct image_t *)mmap(0, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*sharpened_image == MAP_FAILED) {
        perror("mmap sharpened_image");
        exit(1);
    }
    close(shm_fd);

    // Initialize the image structures
    (*smoothened_image)->width = input_image->width;
    (*smoothened_image)->height = input_image->height;
    (*details_image)->width = input_image->width;
    (*details_image)->height = input_image->height;
    (*sharpened_image)->width = input_image->width;
    (*sharpened_image)->height = input_image->height;

    // Set up the image_pixels pointers
    (*smoothened_image)->image_pixels = (uint8_t***)((uint8_t*)*smoothened_image + sizeof(struct image_t));
    (*details_image)->image_pixels = (uint8_t***)((uint8_t*)*details_image + sizeof(struct image_t));
    (*sharpened_image)->image_pixels = (uint8_t***)((uint8_t*)*sharpened_image + sizeof(struct image_t));

    for (int i = 0; i < input_image->height; i++) {
        (*smoothened_image)->image_pixels[i] = (uint8_t**)((uint8_t*)(*smoothened_image)->image_pixels + input_image->height * sizeof(uint8_t**) + i * input_image->width * sizeof(uint8_t*));
        (*details_image)->image_pixels[i] = (uint8_t**)((uint8_t*)(*details_image)->image_pixels + input_image->height * sizeof(uint8_t**) + i * input_image->width * sizeof(uint8_t*));
        (*sharpened_image)->image_pixels[i] = (uint8_t**)((uint8_t*)(*sharpened_image)->image_pixels + input_image->height * sizeof(uint8_t**) + i * input_image->width * sizeof(uint8_t*));
        
        for (int j = 0; j < input_image->width; j++) {
            (*smoothened_image)->image_pixels[i][j] = (uint8_t*)((uint8_t*)(*smoothened_image)->image_pixels + input_image->height * sizeof(uint8_t**) + input_image->height * input_image->width * sizeof(uint8_t*) + (i * input_image->width + j) * 3 * sizeof(uint8_t));
            (*details_image)->image_pixels[i][j] = (uint8_t*)((uint8_t*)(*details_image)->image_pixels + input_image->height * sizeof(uint8_t**) + input_image->height * input_image->width * sizeof(uint8_t*) + (i * input_image->width + j) * 3 * sizeof(uint8_t));
            (*sharpened_image)->image_pixels[i][j] = (uint8_t*)((uint8_t*)(*sharpened_image)->image_pixels + input_image->height * sizeof(uint8_t**) + input_image->height * input_image->width * sizeof(uint8_t*) + (i * input_image->width + j) * 3 * sizeof(uint8_t));
        }
    }

    *sem_s1_s2 = sem_open("/sem_s1_s2", O_CREAT, 0666, 0);
    if (*sem_s1_s2 == SEM_FAILED) {
        perror("sem_open sem_s1_s2");
        exit(1);
    }

    *sem_s2_s3 = sem_open("/sem_s2_s3", O_CREAT, 0666, 0);
    if (*sem_s2_s3 == SEM_FAILED) {
        perror("sem_open sem_s2_s3");
        exit(1);
    }
}

// Function to clean up shared memory and semaphores
void cleanup_shared_resources(sem_t *sem_s1_s2, sem_t *sem_s2_s3) {
    if (sem_close(sem_s1_s2) == -1) {
        perror("sem_close sem_s1_s2");
    }
    if (sem_close(sem_s2_s3) == -1) {
        perror("sem_close sem_s2_s3");
    }
    if (sem_unlink("/sem_s1_s2") == -1) {
        perror("sem_unlink sem_s1_s2");
    }
    if (sem_unlink("/sem_s2_s3") == -1) {
        perror("sem_unlink sem_s2_s3");
    }
    if (shm_unlink("/smoothened_image") == -1) {
        perror("shm_unlink smoothened_image");
    }
    if (shm_unlink("/details_image") == -1) {
        perror("shm_unlink details_image");
    }
    if (shm_unlink("/sharpened_image") == -1) {
        perror("shm_unlink sharpened_image");
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }
    
    // Start timing the overall process
    high_resolution_clock::time_point start_time = high_resolution_clock::now();

    /*// Start timing the overall process
    //high_resolution_clock::time_point start_time = high_resolution_clock::now();
    auto start_time = std::chrono::high_resolution_clock::now();*/

    // Read the input image
    struct image_t *input_image = read_ppm_file(argv[1]);
    if (input_image == nullptr) {
        cerr << "Failed to read input image" << endl;
        return 1;
    }

    // Shared memory for image processing
    struct image_t *smoothened_image;
    struct image_t *details_image;
    struct image_t *sharpened_image;

    // Semaphores for synchronization
    sem_t *sem_s1_s2;
    sem_t *sem_s2_s3;

    // Create shared resources
    create_shared_resources(&smoothened_image, &details_image, &sharpened_image, &sem_s1_s2, &sem_s2_s3, input_image);

    // Fork process for S1
    pid_t pid_s1 = fork();
    if (pid_s1 == -1) {
        perror("fork S1");
        exit(1);
    } else if (pid_s1 == 0) {
        for (int iter = 0; iter < 1000; ++iter) {
            S1_smoothen(input_image, smoothened_image);
            if (sem_post(sem_s1_s2) == -1) {
                perror("sem_post S1");
                exit(1);
            }
        }
        exit(0);
    }

    // Fork process for S2
    pid_t pid_s2 = fork();
    if (pid_s2 == -1) {
        perror("fork S2");
        exit(1);
    } else if (pid_s2 == 0) {
        for (int iter = 0; iter < 1000; ++iter) {
            if (sem_wait(sem_s1_s2) == -1) {
                perror("sem_wait S2");
                exit(1);
            }
            S2_find_details(input_image, smoothened_image, details_image);
            if (sem_post(sem_s2_s3) == -1) {
                perror("sem_post S2");
                exit(1);
            }
        }
        exit(0);
    }

    // Fork process for S3
    pid_t pid_s3 = fork();
    if (pid_s3 == -1) {
        perror("fork S3");
        exit(1);
    } else if (pid_s3 == 0) {
        for (int iter = 0; iter < 1000; ++iter) {
            if (sem_wait(sem_s2_s3) == -1) {
                perror("sem_wait S3");
                exit(1);
            }
            S3_sharpen(input_image, details_image, sharpened_image);
        }
        // Write the final sharpened image after all iterations are complete
        write_ppm_file(argv[2], sharpened_image);
        exit(0);
    }

    // Wait for all child processes to finish
    int status;
    waitpid(pid_s1, &status, 0);
    waitpid(pid_s2, &status, 0);
    waitpid(pid_s3, &status, 0);

    // Clean up shared resources
    cleanup_shared_resources(sem_s1_s2, sem_s2_s3);

    // Free the input image
    free(input_image);

    /*// End timing the overall process
    //high_resolution_clock::time_point end_time = high_resolution_clock::now();
    //microseconds total_duration = duration_cast<seconds>(end_time - start_time);

    // Print the total time taken
    //cout << "Total time: " << total_duration.count()<< " seconds" << endl;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_duration = end_time - start_time;

    // Print the total time taken
    std::cout << "Total time: " << total_duration.count() << " seconds" << std::endl;*/

    // End timing the overall process
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    microseconds total_duration = duration_cast<microseconds>(end_time - start_time);

    // Convert microseconds to seconds (1 second = 1,000,000 microseconds)
    double total_duration_sec = total_duration.count() / 1000000.0;

    // Print the timings in seconds
    cout << "Total time: " << total_duration_sec << " seconds" << endl;


    return 0;
}
