#include "OpenCVCamera.hpp"

bool OpenCVCamera::open() {
    return cap.open(0);
}

bool OpenCVCamera::readFrame(cv::Mat& frame) {
    return cap.read(frame);
}

void OpenCVCamera::close() {
    cap.release();
}