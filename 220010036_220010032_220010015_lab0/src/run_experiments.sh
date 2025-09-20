#!/bin/bash

# Directory structure
LAB_DIR="lab1"
SRC_DIR="$LAB_DIR/src"
IMAGE_DIR="$LAB_DIR/images"

# Change to the src directory
cd $SRC_DIR

# Use make to compile the program
make build-sharpen

# Change back to the original directory
cd ../..

# List of input images
images=("1" "2" "3" "4" "5" "6" "7")

# Number of repetitions for each image
repetitions=5

# Create output TXT file
output_file="results.txt"
> "$output_file"
echo "Image Processing Results" > $output_file
echo "========================" >> $output_file

# Function to calculate average using bc for precision
calculate_average() {
    local sum=0
    for value in "$@"; do
        sum=$(echo "$sum + $value" | bc)
    done
    echo "scale=6; $sum / $#" | bc
}

# Run experiments
for img in "${images[@]}"; do
    echo "Processing $img.ppm..."
    echo "" >> $output_file
    echo "Results for $img.ppm:" >> $output_file
    echo "---------------------" >> $output_file

    file_read_times=()
    s1_times=()
    s2_times=()
    s3_times=()
    file_write_times=()

    for ((i=1; i<=repetitions; i++)); do
        output=$(cd $SRC_DIR && make run-sharpen INPUT=$img OUTPUT="output_$img" 2>&1)
        
        file_read=$(echo "$output" | grep "file read:" | awk '{print $3}')
        s1=$(echo "$output" | grep "S1 (smoothen):" | awk '{print $3}')
        s2=$(echo "$output" | grep "S2 (find details):" | awk '{print $4}')
        s3=$(echo "$output" | grep "S3 (sharpen):" | awk '{print $3}')
        file_write=$(echo "$output" | grep "file write:" | awk '{print $3}')

        file_read_times+=($file_read)
        s1_times+=($s1)
        s2_times+=($s2)
        s3_times+=($s3)
        file_write_times+=($file_write)
    done

    # Calculate averages using bc for floating-point precision
    avg_file_read=$(calculate_average "${file_read_times[@]}")
    avg_s1=$(calculate_average "${s1_times[@]}")
    avg_s2=$(calculate_average "${s2_times[@]}")
    avg_s3=$(calculate_average "${s3_times[@]}")
    avg_file_write=$(calculate_average "${file_write_times[@]}")

    # Write averages to file in seconds
    echo "Average times after $repetitions runs:" >> $output_file
    echo "File read: $avg_file_read seconds" >> $output_file
    echo "S1 (smoothen): $avg_s1 seconds" >> $output_file
    echo "S2 (find details): $avg_s2 seconds" >> $output_file
    echo "S3 (sharpen): $avg_s3 seconds" >> $output_file
    echo "File write: $avg_file_write seconds" >> $output_file
done

echo "Experiments completed. Results saved in $output_file"

