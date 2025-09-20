#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

pid_t rightChildPID = -1;
pid_t leftChildPID = -1;
pid_t searcherPID = -1;
pid_t currentChildPID = -2;

void handleError(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void signalHandler(int signo, siginfo_t *info, void *context) {
    cout << "[" << getpid() << "] received SIGTERM\n";
    
    if (leftChildPID > 0) kill(leftChildPID, SIGTERM);
    if (rightChildPID > 0) kill(rightChildPID, SIGTERM);
    if (searcherPID > 0) kill(searcherPID, SIGTERM);

    exit(0);  // Exit immediately after handling the signal
}

int main(int argc, char **argv) {
    if (argc != 6) {
        cout << "Usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\n";
        return -1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &signalHandler;
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        handleError("sigaction");
    }

    const char *file_to_search_in = argv[1];
    const char *pattern_to_search_for = argv[2];
    int search_start_position = atoi(argv[3]);
    int search_end_position = atoi(argv[4]);
    int max_chunk_size = atoi(argv[5]);
    int midpoint = (search_end_position - search_start_position + 1) / 2;

    cout << "[" << getpid() << "] start position = " << search_start_position << " ; end position = " << search_end_position << "\n";
    
    if (search_end_position - search_start_position + 1 > max_chunk_size) {
        leftChildPID = fork();

        if (leftChildPID < 0) handleError("fork fail");

        if (leftChildPID == 0) {
            cout << "[" << getppid() << "] forked left child " << getpid() << "\n";
            execl("./part3_partitioner.out", "./part3_partitioner.out",
                  file_to_search_in, pattern_to_search_for,
                  to_string(search_start_position).c_str(),
                  to_string(search_start_position + midpoint - 1).c_str(),
                  to_string(max_chunk_size).c_str(), (char *)NULL);
            handleError("execl failed for left child");
        } else {
            rightChildPID = fork();

            if (rightChildPID < 0) handleError("fork fail");

            if (rightChildPID == 0) {
                cout << "[" << getppid() << "] forked right child " << getpid() << "\n";
                execl("./part3_partitioner.out", "./part3_partitioner.out",
                      file_to_search_in, pattern_to_search_for,
                      to_string(search_start_position + midpoint).c_str(),
                      to_string(search_end_position).c_str(),
                      to_string(max_chunk_size).c_str(), (char *)NULL);
                handleError("execl failed for right child");
            } else {
                int status;
                currentChildPID = wait(&status);

                if (currentChildPID == leftChildPID && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    cout << "[" << getpid() << "] left child returned\n";
                    kill(rightChildPID, SIGTERM);
                } else if (currentChildPID == rightChildPID && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    cout << "[" << getpid() << "] right child returned\n";
                    kill(leftChildPID, SIGTERM);
                }
            }
        }
    } else {
        searcherPID = fork();

        if (searcherPID < 0) handleError("fork fail");

        if (searcherPID == 0) {
            cout << "[" << getppid() << "] forked searcher child " << getpid() << "\n";
            execl("./part3_searcher.out", "./part3_searcher.out",
                  file_to_search_in, pattern_to_search_for,
                  to_string(search_start_position).c_str(),
                  to_string(search_end_position).c_str(), (char *)NULL);
            handleError("execl failed for searcher child");
        } else {
            int status;
            currentChildPID = wait(&status);
            cout << "[" << getpid() << "] searcher child returned\n";
        }
    }

    return 0;
}
