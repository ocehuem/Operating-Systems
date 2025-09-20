#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <queue>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <climits> 
using namespace std;

// Structure to hold process information
struct Process {
    string name;                    // Name of the process
    int arrivalTime;                // Arrival time of the process
    vector<int> bursts;             // Vector of CPU and I/O bursts
    int burstIndex = 0;             // Index to track the current CPU burst being processed
    int remainingBurstTime = 0;     // Remaining time for the current CPU burst
    int nextAvailableTime = 0;      // Time when the process is next available for CPU
    int completionTime = 0;         // Time when the process completes its execution
    int totalWaitTime = 0;          // Total waiting time of the process
    int lastCompletionTime = 0;     // Time when the process last executed

    // Constructor for initializing process
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
    // Check if the correct number of arguments are provided
    if (argc != 4) {
        cout << "usage: ./rr.out <scheduling algorithm> <path_to_workload_description_file> <time_quantum>\nprovided arguments:\n";
        for (int i = 0; i < argc; i++)
            cout << argv[i] << "\n";
        return -1;
    }

    int timeQuantum = stoi(argv[3]);
    char *path_to_workload_description_file = argv[2];

    // File reading
    ifstream file(path_to_workload_description_file);

    // Confirm file opening
    if (!file.is_open()) {
        cerr << "Failed to open file: " << path_to_workload_description_file << endl;
        return 1;
    }

    // Variables to store lines and the data matrix
    string line;
    vector<vector<int>> matrix;
    vector<string> line_strings;
    int flags = 0;
    string num;

    // Read the file line by line
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

    // Close the file after reading
    file.close();

    // Convert the lines into integer data
    for (const auto &line_str : line_strings) {
        istringstream iss(line_str);
        vector<int> nums;

        // Use getline to split by space and convert to integers
        while (getline(iss, num, ' ')) {
            if (!num.empty()) {  // Check if the string is not empty
                nums.push_back(stoi(num));
            }
        }

        // Add the row to the matrix
        matrix.push_back(nums);
    }

    // Create processes based on the parsed data
    vector<Process> processes;
    int processCount = 1;
    for (const auto &row : matrix) {
        if (!row.empty()) {
            vector<int> bursts(row.begin() + 1, row.end()); // Get all burst times except the first (arrival time)
            processes.push_back(Process("P" + to_string(processCount), row[0], bursts));
            processCount++;
        }
    }

    // Sort processes based on arrival time
    sort(processes.begin(), processes.end(), [](const Process &a, const Process &b) {
        return a.arrivalTime < b.arrivalTime;
    });

    queue<Process *> readyQueue;  // Queue to hold processes that are ready to execute
    int currentTime = 0;          // Current time tracker
    int completedProcesses = 0;   // Count of completed processes
    int makespan = 0;             // Total makespan of the schedule

    cout << "cpu 0\n";  // CPU start indicator

    // Main scheduling loop
    while (completedProcesses < processes.size()) {
        // Add processes to the ready queue based on current time and arrival time
        for (auto &proc : processes) {
            if (proc.arrivalTime <= currentTime && proc.nextAvailableTime <= currentTime &&
                proc.burstIndex < proc.bursts.size() && proc.burstIndex % 2 == 0) {
                readyQueue.push(&proc);
                proc.nextAvailableTime = INT_MAX;  // Prevent re-adding the process until it's ready again
            }
        }

        if (!readyQueue.empty()) {
            // Get the next process to execute its burst
            Process *currentProcess = readyQueue.front();
            readyQueue.pop();

            // Execute for either the time quantum or the remaining burst time, whichever is smaller
            int timeToExecute = min(timeQuantum, currentProcess->remainingBurstTime);
            int startTime = max(currentTime, currentProcess->arrivalTime); // Start time of the CPU burst
            int endTime = startTime + timeToExecute - 1;                   // End time of this slice

            // Calculate waiting time
            if (startTime > currentProcess->lastCompletionTime) {
             int ioBurstTime = (currentProcess->burstIndex > 1) ? currentProcess->bursts[currentProcess->burstIndex - 1] : 0;
                currentProcess->totalWaitTime += (startTime - currentProcess->lastCompletionTime)-(ioBurstTime+1);
            }

            // Print the burst execution details
            cout << currentProcess->name << "," << (currentProcess->burstIndex / 2) + 1 << " " << startTime << " " << endTime << endl;

            // Update current time and process's remaining burst time
            currentTime = endTime + 1;
            currentProcess->remainingBurstTime -= timeToExecute;
            currentProcess->lastCompletionTime = currentTime;

            // Check if the current CPU burst is done
            if (currentProcess->remainingBurstTime == 0) {
                currentProcess->burstIndex += 2;  // Move to the next CPU burst
                currentProcess->remainingBurstTime = currentProcess->burstIndex < currentProcess->bursts.size() ? currentProcess->bursts[currentProcess->burstIndex] : 0;
                currentProcess->nextAvailableTime = currentTime +
                                                    (currentProcess->burstIndex < currentProcess->bursts.size() ? currentProcess->bursts[currentProcess->burstIndex - 1] : 0);  // Next available time after the I/O burst or at completion
                if (currentProcess->burstIndex >= currentProcess->bursts.size()) {
                    currentProcess->completionTime = currentTime - currentProcess->arrivalTime;  // Adjust to the exact end time without extra increment
                    completedProcesses++;
                    makespan = max(makespan, currentTime);
                }
            } else {
                // If the process hasn't finished, put it back in the queue
                readyQueue.push(currentProcess);
            }
        } else {
            // If no process is ready, increment time to the next arrival
            currentTime++;
        }
    }

    // Print the metrics
    calculateAndPrintMetrics(processes, makespan);

    return 0;
}
