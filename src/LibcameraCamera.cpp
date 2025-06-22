#include "LibcameraCamera.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"
#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/control_ids.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <opencv2/opencv.hpp>

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
    if (camera_->acquire()) {
        log(LogLevel::ERROR, "Failed to acquire camera");
        return false;
    }

    config_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
    config_->at(0).pixelFormat = formats::YUV420;
    config_->at(0).size = { 640, 480 };
    config_->validate();

    if (camera_->configure(config_.get()) < 0) {
        log(LogLevel::ERROR, "Failed to configure camera");
        return false;
    }

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    for (StreamConfiguration &cfg : *config_)
        if (allocator_->allocate(cfg.stream()) < 0) {
            log(LogLevel::ERROR, "Failed to allocate buffers");
            return false;
        }

    for (StreamConfiguration &cfg : *config_) {
        for (const std::unique_ptr<FrameBuffer> &buffer : allocator_->buffers(cfg.stream())) {
            std::unique_ptr<Request> request = camera_->createRequest();
            request->addBuffer(cfg.stream(), buffer.get());
            requests_.push_back(std::move(request));
        }
    }

    camera_->requestCompleted.connect(this, &LibcameraCamera::requestComplete);
    if (camera_->start()) {
        log(LogLevel::ERROR, "Failed to start camera");
        return false;
    }

    for (auto &req : requests_) camera_->queueRequest(req.get());
    log(LogLevel::INFO, "Camera started successfully");
    return true;
}

void LibcameraCamera::requestComplete(Request *request) {
    if (request->status() == Request::RequestCancelled)
        return;

    ScopedTimer timer("Frame Processing");

    for (auto &[stream, buffer] : request->buffers()) {
        const FrameMetadata &metadata = buffer->metadata();
        const FrameBuffer::Plane &plane = buffer->planes()[0];
        int fd = plane.fd.get();
        void *mem = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, fd, 0);
        if (mem != MAP_FAILED) {
            cv::Mat yuv(480 * 3 / 2, 640, CV_8UC1, mem);
            cv::Mat bgr;
            cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);
            cv::imshow("Frame", bgr);
            cv::waitKey(1);
            munmap(mem, plane.length);
        } else {
            log(LogLevel::ERROR, "Failed to mmap buffer");
        }
    }
    camera_->queueRequest(request);
}

void LibcameraCamera::shutdown() {
    log(LogLevel::INFO, "Shutting down camera");
    camera_->stop();
    camera_->release();
    cameraManager_->stop();
}

