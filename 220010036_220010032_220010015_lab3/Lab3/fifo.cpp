#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <bits/stdc++.h>
#include <sstream>
#include <chrono>

using namespace std;

// Structure to hold process information
struct Process {
    string name;                    // Name of the process
    int arrivalTime;                // Arrival time of the process
    vector<int> bursts;             // Vector of CPU and I/O bursts
    int burstIndex = 0;             // Index to track the current CPU burst being processed
    int nextAvailableTime = 0;      // Time when the process is next available for CPU
    int completionTime = 0;         // Time when the process completes its execution
    int totalWaitTime = 0;          // Total waiting time of the process
    int lastCompletionTime = 0;     // Time when the process last executed

    // Constructor for initializing process
    Process(string n, int at, vector<int> b) : name(n), arrivalTime(at), bursts(b) {}
};

// Function to calculate and print metrics
void calculateAndPrintMetrics(const vector<Process> &processes, int makespan) {
    int totalCompletionTime = 0;
    int maxCompletionTime = 0;
    int totalWaitingTime = 0;
    int maxWaitingTime = 0;

    for (const auto &proc : processes) {
        totalCompletionTime += (proc.completionTime);
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
    if (argc != 3) {
        cout << "usage: ./srtf.out <scheduling_algorithm> <path_to_workload_description_file>\nprovided arguments:\n";
        for (int i = 0; i < argc; i++)
            cout << argv[i] << "\n";
        return -1;
    }

    char *scheduling_algorithm = argv[1];
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

    // Output the parsed data to verify
    for (const vector<int> &row : matrix) {
        for (int num : row) {
            //cout << num << " ";  // Print each number in the row
        }
        //cout << endl;  // Move to the next line after each row
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

            // Fetch the burst duration from the process
            int burstDuration = currentProcess->bursts[currentProcess->burstIndex];
            int startTime = max(currentTime, currentProcess->arrivalTime); // Start time of the CPU burst
            int endTime = startTime + burstDuration - 1;                  // End time of the CPU burst

            // Calculate waiting time
            if (startTime > currentProcess->lastCompletionTime) {
             int ioBurstTime = (currentProcess->burstIndex > 1) ? currentProcess->bursts[currentProcess->burstIndex - 1] : 0;
                currentProcess->totalWaitTime += (startTime - currentProcess->lastCompletionTime)-(ioBurstTime+1);
            }

            // Print the burst execution details
            cout << currentProcess->name << "," << (currentProcess->burstIndex / 2) + 1 << " " << startTime << " " << endTime << endl;

            // Update current time and process's next availability
            currentTime = endTime + 1;
            currentProcess->burstIndex += 2;  // Move to the next CPU burst
            currentProcess->lastCompletionTime = currentTime;
            currentProcess->nextAvailableTime = currentTime +
                                                (currentProcess->burstIndex < currentProcess->bursts.size() ? currentProcess->bursts[currentProcess->burstIndex - 1] : 0);  // Next available time after the I/O burst or at completion

            // Update completion time if process has finished all CPU bursts
            if (currentProcess->burstIndex >= currentProcess->bursts.size()) {
            
                currentProcess->completionTime = currentTime- currentProcess->arrivalTime;  // Adjust to the exact end time without extra increment
                completedProcesses++;
                makespan = max(makespan, currentProcess->completionTime);
            }
        } else {
            // If no process is ready, increment time to the next arrival
            currentTime++;
        }
    }

    // Stop measuring execution time
    auto endTime = chrono::high_resolution_clock::now();

    // Print the metrics
    calculateAndPrintMetrics(processes, makespan);

    return 0;
}
