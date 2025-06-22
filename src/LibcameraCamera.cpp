#include "LibcameraCamera.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"
#include <libcamera/control_ids.h>
#include <libcamera/formats.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>

using namespace libcamera;

bool LibcameraCamera::initialize() {
    ScopedTimer timer("Camera Initialization");

    cameraManager_ = std::make_unique<CameraManager>();
    cameraManager_->start();

    if (cameraManager_->cameras().empty()) {
        log(LogLevel::ERROR, "No camera found");
        return false;
    }

    camera_ = cameraManager_->cameras()[0];
    camera_->acquire();

    config_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
    config_->at(0).pixelFormat = formats::YUYV;
    config_->at(0).size.width = 640;
    config_->at(0).size.height = 480;
    config_->validate();
    camera_->configure(config_.get());

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    Stream *stream = config_->at(0).stream();
    if (allocator_->allocate(stream) < 0) {
        log(LogLevel::ERROR, "Failed to allocate buffers");
        return false;
    }

    for (const std::unique_ptr<FrameBuffer> &buffer : allocator_->buffers(stream)) {
        std::unique_ptr<Request> request = camera_->createRequest();
        if (!request) {
            log(LogLevel::ERROR, "Failed to create request");
            continue;
        }
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0) {
            log(LogLevel::ERROR, "Failed to attach buffer to request");
            continue;
        }
        log(LogLevel::INFO, "Buffer attached successfully");
        requests_.push_back(std::move(request));
    }

    camera_->requestCompleted.connect(this, &LibcameraCamera::requestComplete);

    if (camera_->start() < 0) {
        log(LogLevel::ERROR, "Failed to start camera");
        return false;
    }

    for (auto &req : requests_) {
        if (camera_->queueRequest(req.get()) < 0) {
            log(LogLevel::ERROR, "Failed to queue request");
        }
    }

    log(LogLevel::INFO, "Camera started successfully");
    return true;
}

bool LibcameraCamera::captureFrame(cv::Mat &output) {
    ScopedTimer timer("Frame Processing");
    std::unique_lock<std::mutex> lock(frameMutex_);
    frameCondVar_.wait(lock, [this] { return frameReady_; });
    output = latestFrame_.clone();
    frameReady_ = false;
    return true;
}

void LibcameraCamera::shutdown() {
    log(LogLevel::INFO, "Shutting down camera");
    camera_->stop();
    camera_->release();
    cameraManager_->stop();
    log(LogLevel::INFO, "Camera shutdown completed.");
}

void LibcameraCamera::requestComplete(Request *request) {
    if (request->status() == Request::RequestCancelled) {
        log(LogLevel::WARN, "Request was cancelled");
        return;
    }

    const auto &buffers = request->buffers();
    for (auto &[stream, buffer] : buffers) {
        if (buffer->planes().empty()) {
            log(LogLevel::ERROR, "No planes in buffer");
            continue;
        }
        const FrameBuffer::Plane &plane = buffer->planes()[0];
        log(LogLevel::INFO, "Planes count: %zu, length: %u", buffer->planes().size(), plane.length);

        void *memory = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
        if (memory == MAP_FAILED) {
            log(LogLevel::ERROR, "mmap failed");
            continue;
        }

        cv::Mat yuyv(480, 640, CV_8UC2, memory);
        cv::Mat bgr;
        cv::cvtColor(yuyv, bgr, cv::COLOR_YUV2BGR_YUYV);

        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            latestFrame_ = bgr.clone();
            frameReady_ = true;
        }
        frameCondVar_.notify_one();

        munmap(memory, plane.length);
    }

    camera_->queueRequest(request);
}
