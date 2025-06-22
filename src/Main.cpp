#include "LibcameraCamera.hpp"
#include "Logger.hpp"
#include <csignal>
#include <iostream>

LibcameraCamera camera;
bool running = true;

void signalHandler(int signum) {
    log(LogLevel::INFO, "Interrupt signal received. Exiting...");
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);

    if (!camera.initialize()) {
        log(LogLevel::ERROR, "Camera initialization failed");
        return -1;
    }

    log(LogLevel::INFO, "Camera initialized. Press Ctrl+C to exit.");
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    camera.shutdown();
    log(LogLevel::INFO, "Camera shutdown completed.");
    return 0;
}