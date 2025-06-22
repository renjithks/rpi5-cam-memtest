// File: include/LibcameraCamera.hpp
#pragma once
#include <libcamera/libcamera.h>
#include <memory>
#include <vector>

using namespace libcamera;

class LibcameraCamera {
public:
    bool initialize();
    void shutdown();
    bool captureFrame(cv::Mat &output);

private:
    std::unique_ptr<CameraManager> cameraManager_;
    std::shared_ptr<Camera> camera_;
    std::unique_ptr<FrameBufferAllocator> allocator_;
    std::unique_ptr<CameraConfiguration> config_;
    std::vector<std::unique_ptr<Request>> requests_;

    void requestComplete(Request *request);
};
