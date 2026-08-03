#pragma once

#include <opencv2/core.hpp>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <cstddef>

struct ExternalDepthOrientationResult {
    bool horizontalDetermined = false;
    bool verticalDetermined = false;
    bool flippedPointCloudX = false;
    bool flippedPointCloudY = false;
    std::size_t validSamples = 0;
    double columnToPointCloudXCorrelation = 0.0;
    double rowToPointCloudYCorrelation = 0.0;
};

ExternalDepthOrientationResult normalizeExternalDepthPointCloudAxes(
    const cv::Mat& depth,
    pcl::PointCloud<pcl::PointXYZRGB>* cloud);
