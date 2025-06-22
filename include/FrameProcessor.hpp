#pragma once
#include <opencv2/opencv.hpp>

class FrameProcessor {
public:
    void process(const cv::Mat& input, cv::Mat& output);
};