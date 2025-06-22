#include "CameraInterface.hpp"
#include "OpenCVCamera.hpp"
#include "FrameProcessor.hpp"
#include "MemoryManager.hpp"
#include "Profiler.hpp"
#include "Logger.hpp"
#include <iostream>
#include <thread>
#include <cstring>
#include <fstream>

int main() {
    OpenCVCamera camera;
    FrameProcessor processor;
    MemoryManager memManager;
    std::ofstream csvLog("frame_log.csv");
    csvLog << "Frame,AllocSize\n";

    if (!camera.open()) {
        return 1;
    }

    for (int i = 0; i < 300; ++i) {
        ScopedTimer frameTimer("Frame " + std::to_string(i));

        cv::Mat frame, gray;
        if (!camera.readFrame(frame)) {
            log(LogLevel::ERROR, "Frame read failed");
            break;
        }

        processor.process(frame, gray);

        size_t allocSize = gray.total();
        uint8_t* buffer = memManager.allocate(allocSize);
        std::memcpy(buffer, gray.data, allocSize);

        log(LogLevel::INFO, "Frame " + std::to_string(i) + ", Allocated: " + std::to_string(allocSize) + " bytes");
        csvLog << i << "," << allocSize << "\n";

        memManager.free(buffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    camera.close();
    csvLog.close();
    return 0;
}