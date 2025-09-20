#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
#include <algorithm>
using namespace std;

// Structure to hold process information
struct Process {
    string name;
    int arrivalTime;
    vector<int> bursts;  // Assuming only one CPU burst in this case for SJF
    int burstTime;       // This is the first burst (CPU time)
    int completionTime = 0;

    Process(string n, int at, vector<int> b) : name(n), arrivalTime(at), bursts(b), burstTime(b[0]) {}
};

// Function to calculate and print metrics
void calculateAndPrintMetrics(const vector<Process> &processes, int makespan) {
    int totalCompletionTime = 0;
    int maxCompletionTime = 0;

    for (const auto &proc : processes) {
        totalCompletionTime += proc.completionTime;
        maxCompletionTime = max(maxCompletionTime, proc.completionTime);
    }

    cout << "Makespan: " << makespan << endl;
    cout << "Average Completion Time: " << (totalCompletionTime / processes.size()) << endl;
    cout << "Maximum Completion Time: " << maxCompletionTime << endl;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./srtf.out <path_to_workload_description_file>\n";
        return -1;
    }

    char *path_to_workload_description_file = argv[2];

    ifstream file(path_to_workload_description_file);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << path_to_workload_description_file << endl;
        return 1;
    }

    string line;
    vector<vector<int>> matrix;
    vector<string> line_strings;
    int flags = 0;
    string num;

    while (getline(file, line)) {
        if (line == "<pre>") {
            flags = 1;
            continue;
        }
        if (line == "</pre></body></html>") {
            break;
        }
        if (flags == 1) {
            line_strings.push_back(line);
        }
    }

    file.close();

    for (const auto &line_str : line_strings) {
        istringstream iss(line_str);
        vector<int> nums;
        while (getline(iss, num, ' ')) {
            if (!num.empty()) {
                nums.push_back(stoi(num));
            }
        }
        matrix.push_back(nums);
    }

    vector<Process> processes;
    int processCount = 1;
    for (const auto &row : matrix) {
        if (!row.empty()) {
            vector<int> bursts(row.begin() + 1, row.end());
            processes.push_back(Process("P" + to_string(processCount), row[0], bursts));
            processCount++;
        }
    }

    // Sort processes by arrival time
    sort(processes.begin(), processes.end(), [](const Process &a, const Process &b) {
        return a.arrivalTime < b.arrivalTime;
    });

    vector<string> cpu1Log, cpu2Log;  // Logs for CPU 1 and CPU 2
    int cpu1Time = 0, cpu2Time = 0;   // Current times for each CPU
    int completedProcesses = 0;
    int makespan = 0;

    // Process queue that dynamically sorts by burst time (Shortest Job First)
    auto burstTimeComparator = [](Process *a, Process *b) {
        return a->burstTime > b->burstTime;  // Max heap, so we reverse comparison
    };
    priority_queue<Process *, vector<Process *>, decltype(burstTimeComparator)> readyQueue(burstTimeComparator);

    int currentTime = 0;

    while (completedProcesses < processes.size()) {
        // Add processes to the ready queue based on the current time
        for (auto &proc : processes) {
            if (proc.arrivalTime <= currentTime && proc.burstTime > 0) {
                readyQueue.push(&proc);
            }
        }

        // Schedule the next process based on the shortest job first for whichever CPU is available earlier
        if (!readyQueue.empty()) {
            Process *currentProcess = readyQueue.top();
            readyQueue.pop();

            int startTime, endTime;
            if (cpu1Time <= cpu2Time) {
                // Assign process to CPU 1
                startTime = max(cpu1Time, currentProcess->arrivalTime);
                endTime = startTime + currentProcess->burstTime - 1;
                cpu1Log.push_back("CPU 1: " + currentProcess->name + " " + to_string(startTime) + " " + to_string(endTime));
                cpu1Time = endTime + 1;
            } else {
                // Assign process to CPU 2
                startTime = max(cpu2Time, currentProcess->arrivalTime);
                endTime = startTime + currentProcess->burstTime - 1;
                cpu2Log.push_back("CPU 2: " + currentProcess->name + " " + to_string(startTime) + " " + to_string(endTime));
                cpu2Time = endTime + 1;
            }

            currentProcess->completionTime = endTime;
            currentProcess->burstTime = 0;  // Mark the process as completed
            makespan = max(makespan, currentProcess->completionTime);
            completedProcesses++;
        } else {
            // If no process is ready, move the time forward
            currentTime++;
        }
    }

    // Output CPU 1 logs
    for (const auto &log : cpu1Log) {
        cout << log << endl;
    }

    // Output CPU 2 logs
    for (const auto &log : cpu2Log) {
        cout << log << endl;
    }

    calculateAndPrintMetrics(processes, makespan);

    return 0;
}
