#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
#include <climits>
#include <algorithm>
using namespace std;

// Structure to hold process information
struct Process {
    string name;
    int arrivalTime;
    vector<int> bursts;
    int burstIndex = 0;
    int remainingBurstTime = 0;
    int nextAvailableTime = 0;
    int completionTime = 0;
    int totalWaitTime = 0;
    int lastCompletionTime = 0;

    Process(string n, int at, vector<int> b) : name(n), arrivalTime(at), bursts(b), remainingBurstTime(b[0]) {}
};

// Function to calculate and print metrics
void calculateAndPrintMetrics(const vector<Process> &processes, int makespan) {
    int totalCompletionTime = 0;
    int maxCompletionTime = 0;
    int totalWaitingTime = 0;
    int maxWaitingTime = 0;

    for (const auto &proc : processes) {
        totalCompletionTime += proc.completionTime;
        maxCompletionTime = max(maxCompletionTime, proc.completionTime);
        totalWaitingTime += proc.totalWaitTime;
        maxWaitingTime = max(maxWaitingTime, proc.totalWaitTime);
    }

    cout << "Makespan: " << makespan << endl;
    cout << "Average Completion Time: " << (totalCompletionTime / processes.size()) << endl;
    cout << "Maximum Completion Time: " << maxCompletionTime << endl;
    cout << "Average Waiting Time: " << (totalWaitingTime / processes.size()) << endl;
    cout << "Maximum Waiting Time: " << maxWaitingTime << endl;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        cout << "usage: ./rr.out <scheduling algorithm> <path_to_workload_description_file> <time_quantum>\nprovided arguments:\n";
        for (int i = 0; i < argc; i++)
            cout << argv[i] << "\n";
        return -1;
    }

    int timeQuantum = stoi(argv[3]);
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

    sort(processes.begin(), processes.end(), [](const Process &a, const Process &b) {
        return a.arrivalTime < b.arrivalTime;
    });

    queue<Process *> readyQueue;
    int currentTime = 0;
    int completedProcesses = 0;
    int makespan = 0;

    vector<string> cpu1Log, cpu2Log;  // Logs for storing outputs of CPU 1 and CPU 2

    int cpu1Time = 0, cpu2Time = 0;  // Current times for each CPU
    bool cpu1Turn = true;  // Alternates between CPU 1 and CPU 2 for fairness

    // Main scheduling loop
    while (completedProcesses < processes.size()) {
        // Add processes to the ready queue based on current time and arrival time
        for (auto &proc : processes) {
            if (proc.arrivalTime <= currentTime && proc.nextAvailableTime <= currentTime &&
                proc.burstIndex < proc.bursts.size() && proc.burstIndex % 2 == 0) {
                readyQueue.push(&proc);
                proc.nextAvailableTime = INT_MAX;
            }
        }

        if (!readyQueue.empty()) {
            // Alternate between CPU 1 and CPU 2
            Process *currentProcess = readyQueue.front();
            readyQueue.pop();

            int timeToExecute = min(timeQuantum, currentProcess->remainingBurstTime);
            int startTime = max(currentTime, currentProcess->arrivalTime);
            int endTime = startTime + timeToExecute - 1;

            if (startTime > currentProcess->lastCompletionTime) {
                currentProcess->totalWaitTime += (startTime - currentProcess->lastCompletionTime);
            }

            if (cpu1Turn) {
                // Schedule on CPU 1
                cpu1Log.push_back("CPU 1: " + currentProcess->name + "," + to_string((currentProcess->burstIndex / 2) + 1) + " " + to_string(startTime) + " " + to_string(endTime));
                cpu1Time = endTime + 1;
            } else {
                // Schedule on CPU 2
                cpu2Log.push_back("CPU 2: " + currentProcess->name + "," + to_string((currentProcess->burstIndex / 2) + 1) + " " + to_string(startTime) + " " + to_string(endTime));
                cpu2Time = endTime + 1;
            }

            currentProcess->remainingBurstTime -= timeToExecute;
            currentProcess->lastCompletionTime = currentTime;

            if (currentProcess->remainingBurstTime == 0) {
                currentProcess->burstIndex += 2;
                currentProcess->remainingBurstTime = currentProcess->burstIndex < currentProcess->bursts.size() ? currentProcess->bursts[currentProcess->burstIndex] : 0;
                currentProcess->nextAvailableTime = currentTime + (currentProcess->burstIndex < currentProcess->bursts.size() ? currentProcess->bursts[currentProcess->burstIndex - 1] : 0);
                if (currentProcess->burstIndex >= currentProcess->bursts.size()) {
                    currentProcess->completionTime = currentTime - 1;
                    completedProcesses++;
                    makespan = max(makespan, currentProcess->completionTime);
                }
            } else {
                readyQueue.push(currentProcess);
            }

            cpu1Turn = !cpu1Turn;  // Toggle between CPU 1 and CPU 2
        } else {
            currentTime++;
        }
    }

    // Output CPU 1 first
    for (const auto &log : cpu1Log) {
        cout << log << endl;
    }

    // Output CPU 2 next
    for (const auto &log : cpu2Log) {
        cout << log << endl;
    }

    calculateAndPrintMetrics(processes, makespan);

    return 0;
}
