#pragma once
#include <libcamera/libcamera.h>
#include "../dma/DmaBuffer.hpp"
#include "../dma/DmaFrameBuffer.hpp"
#include "../monitoring/MemoryMonitor.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <random>
#include <mutex>
#include <condition_variable>
#include <queue>

class CameraDmaManager {
public:
    CameraDmaManager();
    ~CameraDmaManager();
    
    void configureCamera(const libcamera::Size& size,
                         DmaBufferFactory::AllocationPolicy policy,
                         unsigned bufferCount = 8);
    
    void startCapture();
    void stopCapture();

private:
    void runCaptureLoop();
    void simulateProcessing();
    void logFrameTime(unsigned frameCount);
    void reportStatistics(std::chrono::steady_clock::time_point start, 
                          unsigned frames);
    void handleRequestComplete(libcamera::Request *request);;
    
    std::unique_ptr<DmaBufferFactory> factory_;
    libcamera::CameraManager manager_;
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::vector<std::unique_ptr<DmaFrameBuffer>> buffers_;
    std::vector<std::unique_ptr<libcamera::Request>> requests_;
    std::queue<libcamera::Request *> completedRequests_;
    std::mutex queueMutex_;
    std::condition_variable queueCond_;
    std::atomic<bool> stop_{false};
    std::thread captureThread_;
};