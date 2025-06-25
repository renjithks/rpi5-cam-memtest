#include "MemoryMonitor.hpp"
#include <fstream>
#include <iostream>
#include <cstdio>

MemoryMonitor& MemoryMonitor::instance() {
    static MemoryMonitor singleton;
    return singleton;
}

void MemoryMonitor::startMonitoring(int intervalMs) {
    stop_ = false;
    monitorThread_ = std::thread([this, intervalMs]() {
        while (!stop_) {
            recordMemoryStats();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(intervalMs));
        }
    });
}

void MemoryMonitor::stopMonitoring() {
    stop_ = true;
    if (monitorThread_.joinable()) monitorThread_.join();
}

MemoryMonitor::MemoryStats MemoryMonitor::getCurrentStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return currentStats_;
}

void MemoryMonitor::recordMemoryStats() {
    MemoryStats stats;
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    
    while (std::getline(meminfo, line)) {
        if (line.find("CmaTotal") != std::string::npos) {
            std::sscanf(line.c_str(), "CmaTotal: %lu kB", &stats.cmaTotal);
        } else if (line.find("CmaFree") != std::string::npos) {
            std::sscanf(line.c_str(), "CmaFree: %lu kB", &stats.cmaFree);
        }
    }
    
    if (stats.cmaTotal > 0) {
        stats.fragIndex = 100 - 
            (stats.cmaFree * 100 / stats.cmaTotal);
    }
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        currentStats_ = stats;
    }
    
    logToCsv(stats);
}

void MemoryMonitor::logToCsv(const MemoryStats& stats) {
    static std::ofstream csv("memory_log.csv", 
                           std::ios::out | std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    csv << timestamp << ","
        << stats.cmaTotal << ","
        << stats.cmaFree << ","
        << stats.fragIndex << "\n";
}