#pragma once

#include <opencv2/core.hpp>

// Keeps valid CV_32FC3 depth pixels whose raw Z value is in [zMin, zMax].
// Pixels outside the range (or with Z == 0) are cleared to (0, 0, 0).
cv::Mat FilterByZ(const cv::Mat& xyz, float zMin, float zMax);
