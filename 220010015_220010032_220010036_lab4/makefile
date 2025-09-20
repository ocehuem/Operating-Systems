CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -lpthread -lrt

# Folders for source files
PART1_DIR = part1
PART2_1_DIR = part2_1
PART2_2_DIR = part2_2
PART2_3_DIR = part2_3

# Source files
LIB_SOURCES = libppm.cpp
PART1_SOURCES = $(PART1_DIR)/part1.cpp
PART2_1_SOURCES = $(PART2_1_DIR)/part2_1.cpp
PART2_2_SOURCES = $(PART2_2_DIR)/part2_2.cpp
PART2_3_SOURCES = $(PART2_3_DIR)/part2_3.cpp

# Executable names
PART1_EXE = output_part1
PART2_1_EXE = output_part2_1
PART2_2_EXE = output_part2_2
PART2_3_EXE = output_part2_3

# Default target
all: $(PART1_EXE) $(PART2_1_EXE) $(PART2_2_EXE) $(PART2_3_EXE)

# Build part1
$(PART1_EXE): $(PART1_SOURCES) $(LIB_SOURCES)
	$(CXX) $(PART1_SOURCES) $(LIB_SOURCES) $(CXXFLAGS) -o $(PART1_EXE)

# Build part2_1
$(PART2_1_EXE): $(PART2_1_SOURCES) $(LIB_SOURCES)
	$(CXX) $(PART2_1_SOURCES) $(LIB_SOURCES) $(CXXFLAGS) -o $(PART2_1_EXE)

# Build part2_2
$(PART2_2_EXE): $(PART2_2_SOURCES) $(LIB_SOURCES)
	$(CXX) $(PART2_2_SOURCES) $(LIB_SOURCES) $(CXXFLAGS) -o $(PART2_2_EXE)

# Build part2_3
$(PART2_3_EXE): $(PART2_3_SOURCES) $(LIB_SOURCES)
	$(CXX) $(PART2_3_SOURCES) $(LIB_SOURCES) $(CXXFLAGS) -o $(PART2_3_EXE)

# Run part1, part2_1, part2_2, and part2_3 with user input for image files
run: $(PART1_EXE) $(PART2_1_EXE) $(PART2_2_EXE) $(PART2_3_EXE)
	@read -p "Enter path for input image (eg:1.ppm): " input_image; \
	read -p "Enter path for output image of part1: " output_image1; \
	read -p "Enter path for output image of part2_1: " output_image2; \
	read -p "Enter path for output image of part2_2: " output_image3; \
	read -p "Enter path for output image of part2_3: " output_image4; \
	echo "Running part1..."; ./$(PART1_EXE) $$input_image $$output_image1; \
	echo "Running part2_1..."; ./$(PART2_1_EXE) $$input_image $$output_image2; \
	echo "Running part2_2..."; ./$(PART2_2_EXE) $$input_image $$output_image3; \
	echo "Running part2_3..."; ./$(PART2_3_EXE) $$input_image $$output_image4

# Clean up the build
clean:
	rm -f $(PART1_EXE) $(PART2_1_EXE) $(PART2_2_EXE) $(PART2_3_EXE)
