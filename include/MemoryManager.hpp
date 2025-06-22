#pragma once
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>

class MemoryManager {
public:
    MemoryManager();
    uint8_t* allocate(size_t baseSize);
    void free(uint8_t* ptr);
};