#pragma once
#include <chrono>
#include <string>
#include <iostream>

class ScopedTimer {
    std::string label;
    std::chrono::high_resolution_clock::time_point start;

public:
    ScopedTimer(const std::string& label) : label(label), start(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "[PROFILE] " << label << ": " << durationMs << " ms\n";
    }
};