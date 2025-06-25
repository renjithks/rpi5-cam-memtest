#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <chrono>

class MemoryMonitor {
public:
    static MemoryMonitor& instance();
    
    void startMonitoring(int intervalMs = 1000);
    void stopMonitoring();
    
    struct MemoryStats {
        unsigned long cmaFree = 0;
        unsigned long cmaTotal = 0;
        unsigned long fragIndex = 0;
    };
    
    MemoryStats getCurrentStats();

private:
    MemoryMonitor() = default;
    void recordMemoryStats();
    void logToCsv(const MemoryStats& stats);
    
    std::thread monitorThread_;
    std::atomic<bool> stop_{false};
    std::mutex statsMutex_;
    MemoryStats currentStats_;
};