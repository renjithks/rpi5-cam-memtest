#pragma once
#include "CameraInterface.hpp"

class OpenCVCamera : public CameraInterface {
private:
    cv::VideoCapture cap;
public:
    bool open() override;
    bool readFrame(cv::Mat& frame) override;
    void close() override;
};
