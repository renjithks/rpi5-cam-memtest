#include "LibcameraCamera.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"

using namespace libcamera;

bool LibcameraCamera::initialize() {
    ScopedTimer initTimer("Camera Initialization");
    cameraManager_ = std::make_unique<CameraManager>();
    if (cameraManager_->start()) {
        log(LogLevel::ERROR, "Failed to start CameraManager");
        return false;
    }

    if (cameraManager_->cameras().empty()) {
        log(LogLevel::ERROR, "No cameras found");
        return false;
    }

    camera_ = cameraManager_->cameras()[0];
    if (camera_->acquire()) {
        log(LogLevel::ERROR, "Failed to acquire camera");
        return false;
    }

    config_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
    if (!config_) {
        log(LogLevel::ERROR, "Failed to generate camera configuration");
        return false;
    }

    CameraConfiguration::Status validation = config_->validate();
    if (validation == CameraConfiguration::Invalid) {
        log(LogLevel::ERROR, "Invalid camera configuration");
        return false;
    }
    config_->at(0).pixelFormat = formats::YUV420;
    config_->at(0).size = {640, 480};

    if (camera_->configure(config_.get()) < 0) {
        log(LogLevel::ERROR, "Camera configuration failed");
        return false;
    }

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    StreamConfiguration &cfg = config_->at(0);
    if (allocator_->allocate(cfg.stream()) < 0) {
        log(LogLevel::ERROR, "Failed to allocate buffers");
        return false;
    }

    for (std::unique_ptr<FrameBuffer> &buffer : allocator_->buffers(cfg.stream())) {
        std::unique_ptr<Request> request = camera_->createRequest();
        if (!request) {
            log(LogLevel::ERROR, "Failed to create request");
            return false;
        }
        if (request->addBuffer(cfg.stream(), buffer.get()) < 0) {
            log(LogLevel::ERROR, "Failed to add buffer to request");
            return false;
        }
        requests_.push_back(std::move(request));
    }

    camera_->requestCompleted.connect(this, &LibcameraCamera::requestComplete);

    if (camera_->start()) {
        log(LogLevel::ERROR, "Failed to start camera");
        return false;
    }

    for (auto &req : requests_)
        camera_->queueRequest(req.get());

    log(LogLevel::INFO, "Camera started successfully");
    return true;
}

bool LibcameraCamera::captureFrame(cv::Mat &output) {
    // Placeholder
    return true;
}

void LibcameraCamera::requestComplete(Request *request) {
    // Placeholder
}

void LibcameraCamera::shutdown() {
    if (camera_) camera_->stop();
    if (cameraManager_) cameraManager_->stop();
}
