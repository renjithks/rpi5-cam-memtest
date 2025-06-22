#include "FrameProcessor.hpp"

void FrameProcessor::process(const cv::Mat& input, cv::Mat& output) {
    cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);
}
