#include "LibcameraCamera.hpp"
#include <csignal>
#include <iostream>

LibcameraCamera camera;
bool running = true;

void signalHandler(int signum) {
    std::cout << "\n[INFO] Interrupt signal received. Exiting...\n";
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);

    if (!camera.initialize()) {
        std::cerr << "[ERROR] Camera initialization failed\n";
        return -1;
    }

    std::cout << "[INFO] Camera initialized. Press Ctrl+C to exit.\n";
    while (running) {
        // Just looping until interrupted
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    camera.shutdown();
    std::cout << "[INFO] Camera shutdown completed.\n";
    return 0;
}
