.PHONY: build-part1 run-part1 clean-part1 build-part2 run-part2-searcher run-part2-partitioner clean build-part3 run-part3-searcher run-part3-partitioner

# Part 1
build-part1: part1.out

part1.out: part1_searcher.cpp
	g++ -g part1_searcher.cpp -o part1.out

run-part1: part1.out
	./part1.out file.txt NGTNIJGK 0 67108863

clean-part1:
	rm -f part1.out

# Part 2
build-part2: part2_searcher.out part2_partitioner.out

part2_searcher.out: part2_searcher.cpp
	g++ -g part2_searcher.cpp -o part2_searcher.out

part2_partitioner.out: part2_partitioner.cpp
	g++ -g part2_partitioner.cpp -o part2_partitioner.out

run-part2-searcher: part2_searcher.out
	./part2_searcher.out file.txt NGTNIJGK 0 67108863

run-part2-partitioner: part2_partitioner.out
	./part2_partitioner.out file.txt NGTNIJGK 0 67108863 8388608

# Part 3
build-part3: part3_searcher.out part3_partitioner.out

part3_searcher.out: part3_searcher.cpp
	g++ -g part3_searcher.cpp -o part3_searcher.out

part3_partitioner.out: part3_partitioner.cpp
	g++ -g part3_partitioner.cpp -o part3_partitioner.out

run-part3-searcher: part3_searcher.out
	./part3_searcher.out file.txt NGTNIJGK 0 67108863

run-part3-partitioner: part3_partitioner.out
	./part3_partitioner.out file.txt NGTNIJGK 0 67108863 8388608

# Clean all builds
clean:
	rm -f *.out
