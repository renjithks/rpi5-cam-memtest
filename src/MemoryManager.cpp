#pragma once
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>

class MemoryManager {
public:
    MemoryManager() {
        std::srand(std::time(nullptr));
    }

    uint8_t* allocate(size_t baseSize) {
        size_t randomOffset = std::rand() % 128; // Vary allocation size
        return new uint8_t[baseSize + randomOffset];
    }

    void free(uint8_t* ptr) {
        delete[] ptr;
    }
};
