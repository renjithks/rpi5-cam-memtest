#include "CameraInterface.hpp"
#include "OpenCVCamera.hpp"
#include "FrameProcessor.hpp"
#include "MemoryManager.hpp"
#include <iostream>
#include <thread>
#include <cstring>

int main() {
    OpenCVCamera camera;
    FrameProcessor processor;
    MemoryManager memManager;

    if (!camera.open()) {
        std::cerr << "Failed to open camera\n";
        return 1;
    }

    for (int i = 0; i < 300; ++i) {
        cv::Mat frame, gray;
        if (!camera.readFrame(frame)) {
            std::cerr << "Frame read failed\n";
            break;
        }

        processor.process(frame, gray);

        size_t allocSize = gray.total();
        uint8_t* buffer = memManager.allocate(allocSize);
        std::memcpy(buffer, gray.data, allocSize);

        std::cout << "[INFO] Frame " << i << ", Allocated: " << allocSize << " bytes\n";

        memManager.free(buffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    camera.close();
    return 0;
}
