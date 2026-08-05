#include "filter_by_z.h"

#include <vector>

cv::Mat FilterByZ(const cv::Mat& xyz, float zMin, float zMax)
{
    CV_Assert(xyz.type() == CV_32FC3);
    CV_Assert(zMax >= zMin);

    std::vector<cv::Mat> channels;
    cv::split(xyz, channels);

    cv::Mat atLeastMin;
    cv::Mat atMostMax;
    cv::Mat nonZero;
    cv::Mat valid;
    cv::compare(channels[2], zMin, atLeastMin, cv::CMP_GE);
    cv::compare(channels[2], zMax, atMostMax, cv::CMP_LE);
    cv::compare(channels[2], 0.0f, nonZero, cv::CMP_NE);
    cv::bitwise_and(atLeastMin, atMostMax, valid);
    cv::bitwise_and(valid, nonZero, valid);

    cv::Mat filtered = xyz.clone();
    filtered.setTo(cv::Scalar(0, 0, 0), ~valid);
    return filtered;
}
