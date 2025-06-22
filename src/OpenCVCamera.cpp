#include "OpenCVCamera.hpp"
#include "Logger.hpp"

bool OpenCVCamera::open() {
    bool status = cap.open(0, cv::CAP_LIBCAMERA);
    if (!status) {
        log(LogLevel::ERROR, "Failed to open camera. Ensure camera is connected and permissions are correct.");
        return false;
    }
    log(LogLevel::INFO, "Camera backend: " + cap.getBackendName());
    return true;
}

bool OpenCVCamera::readFrame(cv::Mat& frame) {
    return cap.read(frame);
}

void OpenCVCamera::close() {
    cap.release();
}