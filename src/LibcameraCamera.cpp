#include "LibcameraCamera.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"
#include <libcamera/control_ids.h>
#include <libcamera/formats.h>
#include <libcamera/base/log.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>

using namespace libcamera;

bool LibcameraCamera::initialize() {
    ScopedTimer timer("Camera Initialization");

    cameraManager_ = std::make_unique<CameraManager>();
    if (cameraManager_->start()) {
        log(LogLevel::ERROR, "Failed to start CameraManager");
        return false;
    }

    if (cameraManager_->cameras().empty()) {
        log(LogLevel::ERROR, "No camera found");
        return false;
    }

    camera_ = cameraManager_->cameras()[0];
    if (camera_->acquire()) {
        log(LogLevel::ERROR, "Failed to acquire camera");
        return false;
    }

    config_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
    config_->at(0).pixelFormat = formats::YUV420;
    config_->at(0).size.width = 640;
    config_->at(0).size.height = 480;
    config_->validate();
    if (camera_->configure(config_.get())) {
        log(LogLevel::ERROR, "Failed to configure camera");
        return false;
    }

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
        if (request->addBuffer(stream, buffer.get()) < 0) {
            log(LogLevel::ERROR, "Failed to attach buffer to request");
            continue;
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
    ScopedTimer timer("Frame Processing");
    std::unique_lock<std::mutex> lock(frameMutex_);
    frameCondVar_.wait(lock, [this] { return frameReady_; });
    output = latestFrame_.clone();
    frameReady_ = false;
    return true;
}

void LibcameraCamera::shutdown() {
    log(LogLevel::INFO, "Shutting down camera");
    if (camera_)
        camera_->stop();
    if (camera_)
        camera_->release();
    if (cameraManager_)
        cameraManager_->stop();
    log(LogLevel::INFO, "Camera shutdown completed.");
}

void LibcameraCamera::requestComplete(Request *request) {
    if (request->status() == Request::RequestCancelled) {
        log(LogLevel::WARNING, "Request was cancelled");
        return;
    }

    const std::map<Stream *, FrameBuffer *> &buffers = request->buffers();
    for (auto &[stream, buffer] : buffers) {
        if (buffer->planes().empty()) {
            log(LogLevel::ERROR, "No planes in buffer");
            continue;
        }

        const FrameBuffer::Plane &plane = buffer->planes()[0];
        void *memory = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
        if (memory == MAP_FAILED) {
            log(LogLevel::ERROR, "mmap failed");
            continue;
        }

        try {
            cv::Mat yuv(480 + 480 / 2, 640, CV_8UC1, memory);
            cv::Mat bgr;
            cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);

            {
                std::lock_guard<std::mutex> lock(frameMutex_);
                latestFrame_ = bgr.clone();
                frameReady_ = true;
            }
            frameCondVar_.notify_one();
        } catch (const std::exception &e) {
            log(LogLevel::ERROR, std::string("Exception during frame conversion: ") + e.what());
        }

        munmap(memory, plane.length);
    }

    camera_->queueRequest(request);
}