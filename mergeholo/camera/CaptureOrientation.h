#pragma once

#include <opencv2/core.hpp>

#include <string>

enum class CaptureRotation {
    Clockwise90,
    CounterClockwise90
};

const char* captureRotationName(CaptureRotation rotation);
bool parseCaptureRotation(const std::string& text, CaptureRotation* rotation);

cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source);
cv::Mat rotateCaptureDepthCounterClockwise90(const cv::Mat& source);
