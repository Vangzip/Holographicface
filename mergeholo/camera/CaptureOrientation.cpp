#include "CaptureOrientation.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>

const char* captureRotationName(CaptureRotation rotation)
{
    return rotation == CaptureRotation::CounterClockwise90
        ? "counterclockwise_90"
        : "clockwise_90";
}

bool parseCaptureRotation(const std::string& text, CaptureRotation* rotation)
{
    if (!rotation) {
        return false;
    }

    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    if (normalized == "clockwise_90") {
        *rotation = CaptureRotation::Clockwise90;
        return true;
    }
    if (normalized == "counterclockwise_90") {
        *rotation = CaptureRotation::CounterClockwise90;
        return true;
    }
    return false;
}

cv::Mat rotateCaptureCounterClockwise90(const cv::Mat& source)
{
    if (source.empty()) {
        return {};
    }

    cv::Mat rotated;
    cv::rotate(source, rotated, cv::ROTATE_90_COUNTERCLOCKWISE);
    return rotated;
}

cv::Mat rotateCaptureDepthCounterClockwise90(const cv::Mat& source)
{
    if (source.empty()) {
        return {};
    }

    CV_Assert(source.type() == CV_32FC3);
    cv::Mat rotated = rotateCaptureCounterClockwise90(source);
    for (int row = 0; row < rotated.rows; ++row) {
        cv::Vec3f* points = rotated.ptr<cv::Vec3f>(row);
        for (int column = 0; column < rotated.cols; ++column) {
            const float x = points[column][0];
            const float y = points[column][1];
            points[column][0] = y;
            points[column][1] = -x;
        }
    }
    return rotated;
}
