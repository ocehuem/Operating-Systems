#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <sstream>
#include <climits>

using namespace std;

vector<vector<int>> matrix;

struct Process {
    int id;
    int burst_index;
    int remaining_time;
    int io_completion_time;
    bool operator>(const Process& other) const {
        if (remaining_time == other.remaining_time)
            return id > other.id;
        return remaining_time > other.remaining_time;
    }
};

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./srtf.out <scheduling_algorithm> <path_to_workload_description_file>\n";
        return -1;
    }

    // File reading
    ifstream file(argv[2]);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << argv[2] << endl;
        return 1;
    }

    string line;
    vector<string> line_strings;
    bool start_reading = false;

    while (getline(file, line)) {
        if (line == "<pre>") {
            start_reading = true;
            continue;
        }
        if (line == "</pre></body></html>") {
            break;
        }
        if (start_reading) {
            line_strings.push_back(line);
        }
    }
    file.close();

    // Parse input
    vector<vector<int>> processes;
    for (const auto& str : line_strings) {
        istringstream iss(str);
        vector<int> nums;
        int num;
        while (iss >> num) {
            nums.push_back(num);
        }
        processes.push_back(nums);
    }

    // Initialize metrics tracking
    vector<int> completion_times(processes.size(), 0);
    vector<int> total_cpu_times(processes.size(), 0);
    vector<int> total_io_times(processes.size(), 0);

    // Calculate total CPU and I/O times
    for (size_t i = 0; i < processes.size(); i++) {
        for (size_t j = 1; j < processes[i].size(); j++) {
            if (processes[i][j] == -1) break;
            if (j % 2 == 1) { // CPU burst
                total_cpu_times[i] += processes[i][j];
            } else { // I/O burst
                total_io_times[i] += processes[i][j];
            }
        }
    }

    // Scheduling
    int current_time = 0;
    priority_queue<Process, vector<Process>, greater<Process>> ready_queue;
    vector<int> next_burst_index(processes.size(), 0);
    vector<int> io_completion_times(processes.size(), -1);

    // Variables for output
    int last_process = -1;
    int last_burst_index = -1;
    int burst_start_time = -1;

    while (true) {
        // Check for new arrivals and I/O completions
        for (size_t i = 0; i < processes.size(); i++) {
            if (next_burst_index[i] >= processes[i].size() - 1) continue;
            
            if ((next_burst_index[i] == 0 && processes[i][0] == current_time) ||
                (io_completion_times[i] != -1 && io_completion_times[i] == current_time)) {
                
                ready_queue.push({static_cast<int>(i),
                                 next_burst_index[i]/2,
                                 processes[i][next_burst_index[i] + 1],
                                 -1});
                io_completion_times[i] = -1;
            }
        }

        // Process scheduling
        if (!ready_queue.empty()) {
            Process current = ready_queue.top();
            ready_queue.pop();

            // Check if we need to print previous burst
            if (current.id != last_process || current.burst_index != last_burst_index) {
                if (last_process != -1 && burst_start_time != -1) {
                    cout << "P" << last_process + 1 << "," << last_burst_index + 1 
                         << " " << burst_start_time << " " << current_time - 1 << endl;
                }
                burst_start_time = current_time;
                last_process = current.id;
                last_burst_index = current.burst_index;
            }

            current.remaining_time--;

            if (current.remaining_time == 0) {
                // CPU burst completed
                next_burst_index[current.id] += 2;
                
                // Check if process is completed
                if (next_burst_index[current.id] >= processes[current.id].size() - 1) {
                    completion_times[current.id] = current_time + 1;
                } else {
                    // Set I/O completion time
                    io_completion_times[current.id] = current_time + processes[current.id][next_burst_index[current.id]];
                }
            } else {
                ready_queue.push(current);
            }
        } else {
            // Print last burst if exists
            if (last_process != -1 && burst_start_time != -1) {
                cout << "P" << last_process + 1 << "," << last_burst_index + 1 
                     << " " << burst_start_time << " " << current_time - 1 << endl;
                last_process = -1;
                burst_start_time = -1;
            }
            
            // Check if all processes are completed
            bool all_completed = true;
            for (size_t i = 0; i < processes.size(); i++) {
                if (next_burst_index[i] < processes[i].size() - 1 || io_completion_times[i] != -1) {
                    all_completed = false;
                    break;
                }
            }
            if (all_completed) break;
        }
        
        current_time++;
    }

    // Calculate metrics
    int total_completion_time = 0;
    int max_completion_time = 0;
    int total_waiting_time = 0;
    int max_waiting_time = 0;
    int makespan = current_time - processes[0][0];

    for (size_t i = 0; i < processes.size(); i++) {
        int turnaround_time = completion_times[i] - processes[i][0];
        int waiting_time = turnaround_time - total_cpu_times[i] - total_io_times[i];
        
        total_completion_time += turnaround_time;
        max_completion_time = max(max_completion_time, turnaround_time);
        total_waiting_time += waiting_time;
        max_waiting_time = max(max_waiting_time, waiting_time);
    }

    // Output metrics
    cout << "\nMakespan: " << makespan << endl;
    cout << "Average completion time: " << total_completion_time / processes.size() << endl;
    cout << "Maximum completion time: " << max_completion_time << endl;
    cout << "Average waiting time: " << total_waiting_time / processes.size() << endl;
    cout << "Maximum waiting time: " << max_waiting_time << endl;

    return 0;
}
