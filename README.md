

Worked on a series of C/C++ systems programming projects covering image processing, process management, scheduling, parallelism, and memory management. Built applications such as a PPM image sharpener, a process-based file searcher with inter-process communication, and a scheduler simulator implementing FIFO, SJF, and Round Robin. Designed parallel pipelines using processes, threads, shared memory, pipes, semaphores, and atomic operations, and developed a Virtual Memory Manager with multiple page replacement and allocation policies. Conducted performance analysis, automated builds with Makefiles, and documented results with reports and graphs.

---

## 1 . Breaking the inhibitions(lab0)
- Developed a C/C++ image processing application that sharpens images in PPM format by implementing smoothing, detail extraction, and sharpening transformations.
- Utilized Makefile for automated build and run processes, with debugging handled using gdb and IDE tools.
- Conducted performance analysis using C++ chrono library, measuring execution time across different image sizes and phases (read, S1, S2, S3, write).
- Automated experiments through scripting and documented results in a detailed report.

---

## 2 . Working with processes

- Implemented a process-based file search application in C++, focusing on process creation, execution, and termination using fork, exec, and wait. 
- Designed a partitioned search mechanism where parent processes divide files into chunks and spawn child processes for parallel pattern searching. 
- Enhanced efficiency by implementing signal handling to terminate redundant processes once a match is found. Developed multiple versions (Part I–III) of searcher and partitioner programs, integrating inter-process communication and process tree management. 
- Automated compilation and execution with a structured Makefile. 
- Documented process hierarchies, execution flow, and termination sequences with detailed reports and visualized process trees.

---

## 3 . Process Scheduling

- Developed a C/C++ process scheduling simulator implementing multiple algorithms including FIFO, Non-Preemptive SJF, Preemptive SJF, and Round Robin with variable time quantum.
- Designed efficient data structures for representing processes, bursts, and queues to simulate CPU/IO execution cycles.
- Extended the simulator to evaluate performance on single and dual-processor systems. Computed and analyzed metrics such as makespan, average/max completion time, average/max waiting time, and runtime.
- Automated builds with a Makefile and documented findings through detailed reports and graph-based analyses of algorithm trade-offs.

---

## 4 . Parallel Applications

- Built a parallel image processing application in C/C++ to evaluate different models of parallelism and synchronization.
- Implemented three approaches: (i) multi-process with pipes, (ii) multi-process with shared memory and semaphores, and (iii) multi-threaded with atomic operations.
- Designed a pipeline across three cores where different stages (S1, S2, S3) run concurrently, ensuring ordered and reliable pixel transfer.
- Measured runtime, speedup, and scalability under compute-intensive workloads by repeating transformations 1000 times.
- Compared trade-offs in performance, ease of implementation, and debugging across threads vs. processes and IPC methods.
- Automated compilation with Makefiles and documented results with detailed reports.

---

## 5 . Memory Management

- Developed a Virtual Memory Manager simulator in C++ to study page table management and page replacement policies.
- Implemented Optimal, FIFO, LRU, and Random replacement strategies along with Global and Local allocation policies.
- Designed efficient PageTable and FrameStatus classes to handle virtual-to-physical address mapping for multiple processes on a simulated 64-bit system.
- Conducted experiments with varying page sizes, memory frames, and real-world memory trace files, tracking global and per-process page faults.
- Automated builds with a Makefile and analyzed results using graphs to compare policy performance across workloads.
