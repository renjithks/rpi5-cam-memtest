#include "camera/CameraDmaManager.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        CameraDmaManager manager;
        
        // Test configuration
        libcamera::Size resolution(1920, 1080);
        
        manager.configureCamera(
            resolution, 
            DmaBufferFactory::AllocationPolicy::ALIGNED_1M,
            12  // Buffer count
        );
        
        manager.startCapture();
        
        // Run for 1 hour
        std::this_thread::sleep_for(std::chrono::hours(1));
        
        manager.stopCapture();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}