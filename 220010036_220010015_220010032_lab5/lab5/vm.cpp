#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <cstdint>
#include <random>
#include <algorithm>
#include <sstream>
#include <deque>
#include <iomanip>
#include <limits>

class FrameStatus {
public:
    uint64_t pageNumber;
    int processId;
    int lastUsed;
    int loadTime;
    
    FrameStatus() : pageNumber(0), processId(-1), lastUsed(0), loadTime(0) {}
};

class PageTable {
private:
    std::unordered_map<uint64_t, int> pageToFrame;
    
public:
    void addMapping(uint64_t pageNum, int frameNum) {
        pageToFrame[pageNum] = frameNum;
    }
    
    void removeMapping(uint64_t pageNum) {
        pageToFrame.erase(pageNum);
    }
    
    bool hasMapping(uint64_t pageNum) const {
        return pageToFrame.find(pageNum) != pageToFrame.end();
    }
    
    int getFrame(uint64_t pageNum) const {
        return pageToFrame.at(pageNum);
    }

    void clear() {
        pageToFrame.clear();
    }
};

class VirtualMemoryManager {
private:
    uint64_t pageSize;
    int numFrames;
    std::string replacementPolicy;
    std::string allocationPolicy;
    std::vector<FrameStatus> frames;
    std::vector<PageTable> pageTables;
    int globalPageFaults;
    std::vector<int> processPageFaults;
    int accessCount;
    std::vector<std::pair<int, uint64_t>> futureAccesses;
    int currentAccessIndex;
    std::deque<int> fifoQueue;
    std::vector<int> framesPerProcess;

    uint64_t getPageNumber(uint64_t address) {
        return address / pageSize;
    }

    void initializeFrameAllocation() {
        if (allocationPolicy == "Local") {
            int framesPerProc = numFrames / 4;  // Equal allocation for 4 processes
            framesPerProcess = std::vector<int>(4, framesPerProc);
            int remainingFrames = numFrames % 4;
            for (int i = 0; i < remainingFrames; i++) {
                framesPerProcess[i]++;
            }
        }
    }

    int getProcessFrameCount(int processId) {
        if (allocationPolicy != "Local") return numFrames;
        
        int count = 0;
        for (const auto& frame : frames) {
            if (frame.processId == processId) count++;
        }
        return count;
    }

    bool canProcessAllocateFrame(int processId) {
        if (allocationPolicy != "Local") return true;
        return getProcessFrameCount(processId) < framesPerProcess[processId];
    }
    
    int getFreeFrame(int processId) {
        for (int i = 0; i < numFrames; i++) {
            if (frames[i].processId == -1) {
                if (canProcessAllocateFrame(processId)) {
                    fifoQueue.push_back(i);
                    return i;
                }
            }
        }
        return -1;
    }

    int selectVictimFrame(int currentProcess) {
        if (replacementPolicy == "FIFO") {
            return selectFIFOVictim(currentProcess);
        } else if (replacementPolicy == "LRU") {
            return selectLRUVictim(currentProcess);
        } else if (replacementPolicy == "Random") {
            return selectRandomVictim(currentProcess);
        } else if (replacementPolicy == "Optimal") {
            return selectOptimalVictim(currentProcess);
        }
        return 0;
    }

/*

int selectFIFOVictim(int currentProcess) {
    if (allocationPolicy == "Local") {
        // Ensure FIFO only uses frames belonging to the current process
        while (!fifoQueue.empty()) {
            int frameIndex = fifoQueue.front();
            if (frames[frameIndex].processId == currentProcess) {
                fifoQueue.pop_front();
                fifoQueue.push_back(frameIndex);
                return frameIndex;
            }
            // For FIFO, just pop and re-add the frame to maintain the order
            fifoQueue.pop_front();
        }
        // If no frame is found, allocate a new one
        return getFreeFrame(currentProcess);
    } else {
        // Global FIFO
        int victim = fifoQueue.front();
        fifoQueue.pop_front();
        fifoQueue.push_back(victim);
        return victim;
    }
}

*/

    int selectFIFOVictim(int currentProcess) {
        if (allocationPolicy == "Local") {
            while (!fifoQueue.empty()) {
                int frameIndex = fifoQueue.front();
                if (frames[frameIndex].processId == currentProcess) {
                    fifoQueue.pop_front();
                    fifoQueue.push_back(frameIndex);
                    return frameIndex;
                }
                fifoQueue.push_back(fifoQueue.front());
                fifoQueue.pop_front();
            }
            for (int i = 0; i < numFrames; i++) {
                if (frames[i].processId == currentProcess) {
                    return i;
                }
            }
        } else {
            int victim = fifoQueue.front();
            fifoQueue.pop_front();
            fifoQueue.push_back(victim);
            return victim;
        }
        return 0;
    }

    int selectLRUVictim(int currentProcess) {
        int leastRecentlyUsed = accessCount;
        int victimFrame = 0;
        
        if (allocationPolicy == "Local") {
            for (int i = 0; i < numFrames; i++) {
                if (frames[i].processId == currentProcess && frames[i].lastUsed < leastRecentlyUsed) {
                    leastRecentlyUsed = frames[i].lastUsed;
                    victimFrame = i;
                }
            }
        } else {
            for (int i = 0; i < numFrames; i++) {
                if (frames[i].lastUsed < leastRecentlyUsed) {
                    leastRecentlyUsed = frames[i].lastUsed;
                    victimFrame = i;
                }
            }
        }
        return victimFrame;
    }

    int selectRandomVictim(int currentProcess) {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        if (allocationPolicy == "Local") {
            std::vector<int> processFrames;
            for (int i = 0; i < numFrames; i++) {
                if (frames[i].processId == currentProcess) {
                    processFrames.push_back(i);
                }
            }
            if (processFrames.empty()) return 0;
            std::uniform_int_distribution<> dis(0, processFrames.size() - 1);
            return processFrames[dis(gen)];
        } else {
            std::uniform_int_distribution<> dis(0, numFrames - 1);
            return dis(gen);
        }
    }
/*
    int selectOptimalVictim(int currentProcess) {
        std::vector<std::pair<int, int>> nextUse(numFrames, {std::numeric_limits<int>::max(), 0});
        
        for (int i = 0; i < numFrames; i++) {
            if (allocationPolicy == "Local" && frames[i].processId != currentProcess) {
                continue;
            }
            
            uint64_t pageNum = frames[i].pageNumber;
            int pid = frames[i].processId;
            
            for (int j = currentAccessIndex + 1; j < futureAccesses.size(); j++) {
                if (futureAccesses[j].first == pid && 
                    getPageNumber(futureAccesses[j].second) == pageNum) {
                    nextUse[i] = {j, i};
                    break;
                }
            }
        }
        
        auto maxElement = std::max_element(nextUse.begin(), nextUse.end());
        return maxElement->second;
    }
*/

int selectOptimalVictim(int currentProcess) {
    std::vector<std::pair<int, int>> nextUse(numFrames, {std::numeric_limits<int>::max(), 0});
    
    // Check next use for each frame
    for (int i = 0; i < numFrames; i++) {
        if (allocationPolicy == "Local" && frames[i].processId != currentProcess) {
            continue;
        }
        
        uint64_t pageNum = frames[i].pageNumber;
        int pid = frames[i].processId;
        
        bool foundFutureUse = false;
        for (int j = currentAccessIndex + 1; j < futureAccesses.size(); j++) {
            if (futureAccesses[j].first == pid && getPageNumber(futureAccesses[j].second) == pageNum) {
                nextUse[i] = {j, i};
                foundFutureUse = true;
                break;
            }
        }
        
        // If no future use found, set the next use to a very large value (meaning it's least preferred)
        if (!foundFutureUse) {
            nextUse[i] = {std::numeric_limits<int>::max(), i};
        }
    }

    // Find the frame with the farthest next use or no future use at all
    auto maxElement = std::max_element(nextUse.begin(), nextUse.end());
    return maxElement->second;
}

public:
    VirtualMemoryManager(uint64_t pageSize, int numFrames, 
                        std::string replacementPolicy, std::string allocationPolicy)
        : pageSize(pageSize), numFrames(numFrames), 
          replacementPolicy(replacementPolicy), allocationPolicy(allocationPolicy),
          frames(numFrames), pageTables(4), 
          globalPageFaults(0), processPageFaults(4, 0),
          accessCount(0), currentAccessIndex(0) {
        initializeFrameAllocation();
    }
    
    void loadTrace(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            int pid;
            uint64_t addr;
            char comma;
            std::istringstream iss(line);
            iss >> pid >> comma >> addr;
            futureAccesses.push_back({pid, addr});
        }
    }
    
    void processAccess(int processId, uint64_t address) {
        uint64_t pageNum = getPageNumber(address);
        currentAccessIndex = accessCount;
        
        if (!pageTables[processId].hasMapping(pageNum)) {
            // Page fault occurred
            globalPageFaults++;
            processPageFaults[processId]++;
            
            int frameNum = getFreeFrame(processId);
            
            if (frameNum == -1) {
                frameNum = selectVictimFrame(processId);
                if (frames[frameNum].processId != -1) {
                    pageTables[frames[frameNum].processId].removeMapping(frames[frameNum].pageNumber);
                }
            }
            
            frames[frameNum].pageNumber = pageNum;
            frames[frameNum].processId = processId;
            frames[frameNum].lastUsed = accessCount;
            frames[frameNum].loadTime = accessCount;
            pageTables[processId].addMapping(pageNum, frameNum);
            
            /*std::cout << "Page Fault: Process " << processId << ", Virtual Page " << pageNum
                      << ", Mapped to Frame " << frameNum << "\n";*/
        } else {
            int frameNum = pageTables[processId].getFrame(pageNum);
            frames[frameNum].lastUsed = accessCount;
            /*std::cout << "Page Hit: Process " << processId << ", Virtual Page " << pageNum
                      << ", Located in Frame " << frameNum << "\n";*/
        }
        
        accessCount++;
    }
    
    void simulate(const std::string& traceFile) {
        loadTrace(traceFile);
        
        for (auto& table : pageTables) {
            table.clear();
        }
        for (auto& frame : frames) {
            frame = FrameStatus();
        }
        fifoQueue.clear();
        globalPageFaults = 0;
        std::fill(processPageFaults.begin(), processPageFaults.end(), 0);
        accessCount = 0;
        currentAccessIndex = 0;
        
        std::ifstream file(traceFile);
        std::string line;
        
        while (std::getline(file, line)) {
            int pid;
            uint64_t addr;
            char comma;
            std::istringstream iss(line);
            iss >> pid >> comma >> addr;
            processAccess(pid, addr);
        }
        
        // Print results
        std::cout << "\nReplacement Policy: " << replacementPolicy << std::endl;
        std::cout << "Allocation Policy: " << allocationPolicy << std::endl;
        std::cout << "Page Size: " << pageSize << " bytes" << std::endl;
        std::cout << "Number of Frames: " << numFrames << std::endl;
        std::cout << "Total page faults: " << globalPageFaults << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "Process " << i << " page faults: " << processPageFaults[i] << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <page-size> <number-of-memory-frames> "
                 << "<replacement-policy> <allocation-policy> <trace-file>" << std::endl;
        return 1;
    }
    
    uint64_t pageSize = std::stoull(argv[1]);
    int numFrames = std::stoi(argv[2]);
    std::string replacementPolicy = argv[3];
    std::string allocationPolicy = argv[4];
    std::string traceFile = argv[5];
    
    VirtualMemoryManager vmm(pageSize, numFrames, replacementPolicy, allocationPolicy);
    vmm.simulate(traceFile);
    
    return 0;
}
