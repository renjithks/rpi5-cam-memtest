#pragma once
#include <opencv2/opencv.hpp>

class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    virtual bool open() = 0;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual void close() = 0;
};