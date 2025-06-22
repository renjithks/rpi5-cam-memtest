#include "MemoryManager.hpp"

MemoryManager::MemoryManager() {
    std::srand(std::time(nullptr));
}

uint8_t* MemoryManager::allocate(size_t baseSize) {
    size_t randomOffset = std::rand() % 128; // Simulate fragmentation
    return new uint8_t[baseSize + randomOffset];
}

void MemoryManager::free(uint8_t* ptr) {
    delete[] ptr;
}