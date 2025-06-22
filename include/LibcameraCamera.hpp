#pragma once
#include <memory>
#include <opencv2/opencv.hpp>

class LibcameraCamera {
public:
    bool initialize();
    bool captureFrame(cv::Mat &frame);
    void shutdown();
};
