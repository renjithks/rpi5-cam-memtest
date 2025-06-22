#pragma once

#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>

class LibcameraCamera {
public:
    bool initialize();
    bool captureFrame(cv::Mat &output);
    void shutdown();

private:
    std::unique_ptr<libcamera::CameraManager> cameraManager_;
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;
    std::vector<std::unique_ptr<libcamera::Request>> requests_;

    std::mutex frameMutex_;
    std::condition_variable frameCondVar_;
    cv::Mat latestFrame_;
    bool frameReady_ = false;

    void requestComplete(libcamera::Request *request);
};
