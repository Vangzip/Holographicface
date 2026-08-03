#include "ExternalDepthOrientation.h"

#include <cmath>

namespace {

struct CorrelationAccumulator {
    long double sumA = 0.0;
    long double sumB = 0.0;
    long double sumASquared = 0.0;
    long double sumBSquared = 0.0;
    long double sumAB = 0.0;

    void add(long double a, long double b)
    {
        sumA += a;
        sumB += b;
        sumASquared += a * a;
        sumBSquared += b * b;
        sumAB += a * b;
    }

    bool correlation(std::size_t sampleCount, double* value) const
    {
        if (sampleCount < 8 || value == nullptr) {
            return false;
        }

        const long double count = static_cast<long double>(sampleCount);
        const long double covariance = count * sumAB - sumA * sumB;
        const long double varianceA = count * sumASquared - sumA * sumA;
        const long double varianceB = count * sumBSquared - sumB * sumB;
        if (varianceA <= 0.0 || varianceB <= 0.0) {
            return false;
        }

        const long double denominator = std::sqrt(varianceA * varianceB);
        *value = static_cast<double>(covariance / denominator);
        return std::isfinite(*value) && std::fabs(*value) >= 0.5;
    }
};

} // namespace

ExternalDepthOrientationResult normalizeExternalDepthPointCloudAxes(
    const cv::Mat& depth,
    pcl::PointCloud<pcl::PointXYZRGB>* cloud)
{
    ExternalDepthOrientationResult result;
    if (cloud == nullptr || depth.empty() || depth.type() != CV_32FC3) {
        return result;
    }

    CorrelationAccumulator horizontal;
    CorrelationAccumulator vertical;

    for (int row = 0; row < depth.rows; ++row) {
        for (int column = 0; column < depth.cols; ++column) {
            const cv::Vec3f xyz = depth.at<cv::Vec3f>(row, column);
            if (!std::isfinite(xyz[0]) || !std::isfinite(xyz[1])
                || !std::isfinite(xyz[2]) || std::fabs(xyz[2]) < 1e-6f) {
                continue;
            }

            horizontal.add(
                static_cast<long double>(column),
                static_cast<long double>(xyz[0]));
            vertical.add(
                static_cast<long double>(row),
                -static_cast<long double>(xyz[1]));
            ++result.validSamples;
        }
    }

    result.horizontalDetermined = horizontal.correlation(
        result.validSamples, &result.columnToPointCloudXCorrelation);
    result.verticalDetermined = vertical.correlation(
        result.validSamples, &result.rowToPointCloudYCorrelation);
    result.flippedPointCloudX = result.horizontalDetermined
        && result.columnToPointCloudXCorrelation < 0.0;
    result.flippedPointCloudY = result.verticalDetermined
        && result.rowToPointCloudYCorrelation > 0.0;

    if (result.flippedPointCloudX || result.flippedPointCloudY) {
        for (pcl::PointXYZRGB& point : *cloud) {
            if (result.flippedPointCloudX) {
                point.x = -point.x;
            }
            if (result.flippedPointCloudY) {
                point.y = -point.y;
            }
        }
    }
    return result;
}
