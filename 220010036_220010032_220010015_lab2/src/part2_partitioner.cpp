#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>

using namespace std;

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        cout << "usage: ./part2_partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\nprovided arguments:\n";
        for (int i = 0; i < argc; i++)
            cout << argv[i] << "\n";
        return -1;
    }

    char *file_to_search_in = argv[1];
    char *pattern_to_search_for = argv[2];
    int search_start_position = atoi(argv[3]);
    int search_end_position = atoi(argv[4]);
    int max_chunk_size = atoi(argv[5]);

    pid_t my_pid = getpid();
    pid_t left_child_pid, right_child_pid;

    cout << "[" << my_pid << "] start position = " << search_start_position << " ; end position = " << search_end_position << "\n";

    if (search_end_position - search_start_position <= max_chunk_size)
    {
        // Search in this chunk
        pid_t searcher_pid = fork();
        if (searcher_pid == 0)
        {
            // Child process
            execl("./part2_searcher.out", "./part2_searcher.out",
                  file_to_search_in, pattern_to_search_for,
                  to_string(search_start_position).c_str(),
                  to_string(search_end_position).c_str(),
                  (char *)NULL);
            perror("execl");
            exit(1);
        }
        else if (searcher_pid > 0)
        {
            cout << "[" << my_pid << "] forked searcher child " << searcher_pid << "\n";
            wait(NULL); // Wait for the child process to finish
            cout << "[" << my_pid << "] searcher child returned\n";
        }
        else
        {
            perror("fork");
            exit(1);
        }
    }
    else
    {
        // Split into two chunks
        int mid = (search_start_position + search_end_position) / 2;

        left_child_pid = fork();
        if (left_child_pid == 0)
        {
            // Left child
            execl("./part2_partitioner.out", "./part2_partitioner.out",
                  file_to_search_in, pattern_to_search_for,
                  to_string(search_start_position).c_str(),
                  to_string(mid).c_str(),
                  to_string(max_chunk_size).c_str(),
                  (char *)NULL);
            perror("execl");
            exit(1);
        }
        else if (left_child_pid > 0)
        {
            cout << "[" << my_pid << "] forked left child " << left_child_pid << "\n";

            right_child_pid = fork();
            if (right_child_pid == 0)
            {
                // Right child
                execl("./part2_partitioner.out", "./part2_partitioner.out",
                      file_to_search_in, pattern_to_search_for,
                      to_string(mid + 1).c_str(),
                      to_string(search_end_position).c_str(),
                      to_string(max_chunk_size).c_str(),
                      (char *)NULL);
                perror("execl");
                exit(1);
            }
            else if (right_child_pid > 0)
            {
                cout << "[" << my_pid << "] forked right child " << right_child_pid << "\n";

                // Wait for both child processes to finish
                waitpid(left_child_pid, NULL, 0);
                cout << "[" << my_pid << "] left child returned\n";

                waitpid(right_child_pid, NULL, 0);
                cout << "[" << my_pid << "] right child returned\n";
            }
            else
            {
                perror("fork");
                exit(1);
            }
        }
        else
        {
            perror("fork");
            exit(1);
        }
    }

    return 0;
}

