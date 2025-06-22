// LibcameraCamera.cpp
#include "LibcameraCamera.hpp"
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
    cameraManager_ = std::make_unique<CameraManager>();
    cameraManager_->start();

    if (cameraManager_->cameras().empty()) {
        std::cerr << "[ERROR] No camera found\n";
        return false;
    }

    camera_ = cameraManager_->cameras()[0];
    if (camera_->acquire()) {
        std::cerr << "[ERROR] Failed to acquire camera\n";
        return false;
    }

    config_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
    config_->at(0).pixelFormat = formats::YUV420;
    config_->at(0).size = { 640, 480 };
    config_->validate();

    if (camera_->configure(config_.get()) < 0) {
        std::cerr << "[ERROR] Failed to configure camera\n";
        return false;
    }

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    for (StreamConfiguration &cfg : *config_)
        if (allocator_->allocate(cfg.stream()) < 0) {
            std::cerr << "[ERROR] Failed to allocate buffers\n";
            return false;
        }

    for (StreamConfiguration &cfg : *config_) {
        for (std::unique_ptr<FrameBuffer> &buffer : allocator_->buffers(cfg.stream())) {
            std::unique_ptr<Request> request = camera_->createRequest();
            request->addBuffer(cfg.stream(), buffer.get());
            requests_.push_back(std::move(request));
        }
    }

    camera_->requestCompleted.connect(this, &LibcameraCamera::requestComplete);
    if (camera_->start()) {
        std::cerr << "[ERROR] Failed to start camera\n";
        return false;
    }

    for (auto &req : requests_) camera_->queueRequest(req.get());
    return true;
}

void LibcameraCamera::requestComplete(Request *request) {
    if (request->status() == Request::RequestCancelled)
        return;

    for (auto &[stream, buffer] : request->buffers()) {
        const FrameMetadata &metadata = buffer->metadata();
        const FrameBuffer::Plane &plane = buffer->planes()[0];
        int fd = plane.fd.fd();
        void *mem = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, fd, 0);
        if (mem != MAP_FAILED) {
            cv::Mat yuv(480 * 3 / 2, 640, CV_8UC1, mem);
            cv::Mat bgr;
            cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_I420);
            cv::imshow("Frame", bgr);
            cv::waitKey(1);
            munmap(mem, plane.length);
        }
    }
    camera_->queueRequest(request);
}

void LibcameraCamera::shutdown() {
    camera_->stop();
    camera_->release();
    cameraManager_->stop();
}