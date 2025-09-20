CXX = g++
SRC = vm.cpp
OBJ = vm.o
TARGET = vm

SIZE = 4096
FRAMES = 4
INPUT = combined.trace

$(OBJ): $(SRC)
	$(CXX) -c $(SRC)

$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET) $(SIZE) $(FRAMES) FIFO Global $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) FIFO Local $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) LRU Global $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) LRU Local $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) Random Global $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) Random Local $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) Optimal Global $(INPUT)
	./$(TARGET) $(SIZE) $(FRAMES) Optimal Local $(INPUT)
	
.PHONY: all clean run

all: $(TARGET) run

