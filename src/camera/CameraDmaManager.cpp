#include "CameraDmaManager.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <stdexcept>

CameraDmaManager::CameraDmaManager() 
    : factory_(std::make_unique<DmaBufferFactory>()) {}

CameraDmaManager::~CameraDmaManager() {
    stopCapture();
}

void CameraDmaManager::configureCamera(const libcamera::Size& size,
                                       DmaBufferFactory::AllocationPolicy policy,
                                       unsigned bufferCount) {
    manager_.start();
    if (manager_.cameras().empty()) {
        throw std::runtime_error("No cameras available");
    }
    
    camera_ = manager_.get(manager_.cameras()[0]->id());
    camera_->acquire();
    
    config_ = camera_->generateConfiguration({ libcamera::StreamRole::VideoRecording });
    config_->at(0).size = size;
    config_->at(0).pixelFormat = libcamera::formats::YUV420;
    
    if (config_->validate() != libcamera::CameraConfiguration::Valid) {
        throw std::runtime_error("Camera configuration invalid");
    }
    
    camera_->configure(config_.get());
    
    const size_t bufferSize = config_->at(0).frameSize;
    buffers_.reserve(bufferCount);
    
    for (unsigned i = 0; i < bufferCount; i++) {
        auto dmaBuf = factory_->createBuffer(bufferSize, policy);
        auto frameBuf = std::make_unique<DmaFrameBuffer>(dmaBuf, config_->at(0));
        buffers_.push_back(std::move(frameBuf));
    }
}

void CameraDmaManager::startCapture() {

    camera_->requestCompleted.connect(this, &CameraDmaManager::handleRequestComplete);
    stop_ = false;
    camera_->start();
    
    // Create and queue requests
    for (unsigned i = 0; i < buffers_.size(); i++) {
        auto request = camera_->createRequest(i);
        if (!request) {
            throw std::runtime_error("Failed to create request");
        }
        
        if (request->addBuffer(config_->at(0).stream(), buffers_[i]->get()) < 0) {
            throw std::runtime_error("Failed to add buffer to request");
        }
        
        requests_.push_back(std::move(request));
        camera_->queueRequest(requests_[i].get());
    }
    
    MemoryMonitor::instance().startMonitoring();
    captureThread_ = std::thread(&CameraDmaManager::runCaptureLoop, this);
}

void CameraDmaManager::stopCapture() {
    stop_ = true;
    
    if (captureThread_.joinable()) {
        captureThread_.join();
    }
    
    if (camera_) {
        camera_->stop();
        camera_->release();
    }
    
    manager_.stop();
    MemoryMonitor::instance().stopMonitoring();
}

void CameraDmaManager::runCaptureLoop() {
    unsigned frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (!stop_) {
        libcamera::Request *request = nullptr;

        // Wait for completed request
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCond_.wait(lock, [this]() { return !completedRequests_.empty() || stop_; });

            if (stop_) break;

            request = completedRequests_.front();
            completedRequests_.pop();
        }

        frameCount++;

        auto frameBuffer = buffers_[request->cookie()];
        frameBuffer->dmaBuffer()->sync(true);

        simulateProcessing();

        frameBuffer->dmaBuffer()->sync(false);

        request->reuse(libcamera::Request::ReuseBuffers);
        camera_->queueRequest(request);

        if (frameCount % 100 == 0) {
            logFrameTime(frameCount);
        }
    }

    reportStatistics(startTime, frameCount);
}

void CameraDmaManager::handleRequestComplete(libcamera::Request *request) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    completedRequests_.push(request);
    queueCond_.notify_one();
}

void CameraDmaManager::simulateProcessing() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(500, 5000);
    std::this_thread::sleep_for(std::chrono::microseconds(dist(rng)));
}

void CameraDmaManager::logFrameTime(unsigned frameCount) {
    static auto lastFrameTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime);
    lastFrameTime = now;

    static std::ofstream log("frame_times.csv", std::ios::app);
    log << frameCount << "," << frameTime.count() << "\n";
}

void CameraDmaManager::reportStatistics(std::chrono::steady_clock::time_point start, 
                                       unsigned frames) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start);
    auto stats = MemoryMonitor::instance().getCurrentStats();

    std::cout << "\n===== Capture Report =====\n"
              << "Frames captured: " << frames << "\n"
              << "Duration: " << duration.count() << " seconds\n"
              << "Avg FPS: " << (duration.count() > 0 ? 
                 static_cast<float>(frames) / duration.count() : 0) << "\n"
              << "CMA Usage: " << stats.cmaFree << "/" 
              << stats.cmaTotal << " KB free\n"
              << "Fragmentation Index: " << stats.fragIndex << "%\n";
}