#ifndef FRAME_CHANGE_DETECTOR_HPP
#define FRAME_CHANGE_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

// ==================== 配置参数 ====================
const double MOVEMENT_THRESHOLD = 25.0;      // 帧间差分阈值
const int MIN_MOVEMENT_AREA = 11000;           // 最小运动区域面积
const int MIN_CONTOUR_AREA = 50;             // 最小轮廓面积（过滤小噪声）
const cv::Size GAUSSIAN_BLUR_SIZE(5, 5);     // 高斯模糊核大小
const cv::Size MORPH_KERNEL_SIZE(5, 5);      // 形态学操作核大小
// ==================== 配置参数结束 ====================

// 辅助函数：保存比较结果
inline void saveComparisonResult(const cv::Mat& frame1, const cv::Mat& frame2,
                          double movementArea, bool hasChange) {
    // 创建输出目录
    system("mkdir -p comparison_results");

    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", std::localtime(&time));
    std::string timestamp = buffer;

    // 保存原始帧
    cv::imwrite("comparison_results/frame1_" + timestamp + ".jpg", frame1);
    cv::imwrite("comparison_results/frame2_" + timestamp + ".jpg", frame2);

    // 创建比较报告
    std::string report = "comparison_results/report_" + timestamp + ".txt";
    FILE* fp = fopen(report.c_str(), "w");
    if (fp) {
        fprintf(fp, "两帧图像变化检测报告\n");
        fprintf(fp, "时间: %s\n", timestamp.c_str());
        fprintf(fp, "检测结果: %s\n", hasChange ? "有显著变化" : "无显著变化");
        fprintf(fp, "运动区域面积: %.2f 像素\n", movementArea);
        fprintf(fp, "阈值: %d\n", MIN_MOVEMENT_AREA);
        fprintf(fp, "帧1尺寸: %dx%d, 通道: %d\n",
                frame1.cols, frame1.rows, frame1.channels());
        fprintf(fp, "帧2尺寸: %dx%d, 通道: %d\n",
                frame2.cols, frame2.rows, frame2.channels());
        fclose(fp);
    }

    std::cout << "结果已保存到 comparison_results 目录" << std::endl;
}

// 辅助函数：可视化两帧比较结果
inline void visualizeTwoFrameComparison(const cv::Mat& frame1, const cv::Mat& frame2,
                                 const cv::Mat& frameDiff, const cv::Mat& thresholdDiff,
                                 double movementArea, bool hasChange) {

    // 创建对比显示图像
    cv::Mat comparisonDisplay;

    // 调整图像大小以便并排显示
    int displayWidth = 800;
    int displayHeight = 600;

    // 创建显示帧1和帧2
    cv::Mat displayFrame1, displayFrame2;
    cv::resize(frame1, displayFrame1, cv::Size(displayWidth/2, displayHeight/2));
    cv::resize(frame2, displayFrame2, cv::Size(displayWidth/2, displayHeight/2));

    // 创建显示差分图像
    cv::Mat displayDiff, diffColor;
    cv::normalize(frameDiff, displayDiff, 0, 255, cv::NORM_MINMAX);
    displayDiff.convertTo(displayDiff, CV_8U);
    cv::applyColorMap(displayDiff, diffColor, cv::COLORMAP_JET);
    cv::resize(diffColor, diffColor, cv::Size(displayWidth/2, displayHeight/2));

    // 创建显示二值化图像
    cv::Mat displayThreshold, thresholdColor;
    cv::cvtColor(thresholdDiff, thresholdColor, cv::COLOR_GRAY2BGR);
    cv::resize(thresholdColor, thresholdColor, cv::Size(displayWidth/2, displayHeight/2));

    // 将4个图像组合成一个显示图像
    cv::Mat topRow, bottomRow;
    cv::hconcat(displayFrame1, displayFrame2, topRow);
    cv::hconcat(diffColor, thresholdColor, bottomRow);
    cv::vconcat(topRow, bottomRow, comparisonDisplay);

    // 添加文字说明
    std::string status = hasChange ? "Change Detected" : "No Significant Change";
    cv::Scalar statusColor = hasChange ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);

    cv::putText(comparisonDisplay, "Frame 1", cv::Point(10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparisonDisplay, "Frame 2", cv::Point(displayWidth/2 + 10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparisonDisplay, "Frame Diff", cv::Point(10, displayHeight/2 + 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    cv::putText(comparisonDisplay, "Threshold", cv::Point(displayWidth/2 + 10, displayHeight/2 + 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

    // 显示状态信息
    cv::putText(comparisonDisplay, "Status: " + status,
                cv::Point(10, displayHeight - 50),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, statusColor, 2);

    cv::putText(comparisonDisplay, "Movement Area: " + std::to_string((int)movementArea) + " px",
                cv::Point(10, displayHeight - 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);

    // 显示图像
    cv::imshow("Frame Change Detection", comparisonDisplay);
}

// 独立的测试函数：判断两个图像之间是否有显著变化
inline bool hasSignificantChange(const cv::Mat& frame1, const cv::Mat& frame2,
                          bool showDebug = false, double* movementArea = nullptr) {

    // 检查输入图像是否有效
    if (frame1.empty() || frame2.empty()) {
        std::cerr << "错误: 输入图像为空!" << std::endl;
        return false;
    }

    // 检查图像大小是否一致
    if (frame1.size() != frame2.size()) {
        std::cerr << "警告: 两帧图像大小不一致，正在调整..." << std::endl;
        cv::Mat frame2_resized;
        cv::resize(frame2, frame2_resized, frame1.size());
        return hasSignificantChange(frame1, frame2_resized, showDebug, movementArea);
    }

    // 转换为灰度图
    cv::Mat gray1, gray2;
    if (frame1.channels() == 3) {
        cv::cvtColor(frame1, gray1, cv::COLOR_BGR2GRAY);
    } else {
        gray1 = frame1.clone();
    }

    if (frame2.channels() == 3) {
        cv::cvtColor(frame2, gray2, cv::COLOR_BGR2GRAY);
    } else {
        gray2 = frame2.clone();
    }

    // 应用高斯模糊减少噪声
    cv::GaussianBlur(gray1, gray1, GAUSSIAN_BLUR_SIZE, 0);
    cv::GaussianBlur(gray2, gray2, GAUSSIAN_BLUR_SIZE, 0);

    // 计算帧间差分
    cv::Mat frameDiff;
    cv::absdiff(gray1, gray2, frameDiff);

    // 二值化
    cv::Mat thresholdDiff;
    cv::threshold(frameDiff, thresholdDiff, MOVEMENT_THRESHOLD, 255, cv::THRESH_BINARY);

    // 形态学操作
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, MORPH_KERNEL_SIZE);
    cv::morphologyEx(thresholdDiff, thresholdDiff, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(thresholdDiff, thresholdDiff, cv::MORPH_OPEN, kernel);

    // 查找轮廓并计算运动区域总面积
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresholdDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double totalMovementArea = 0;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > MIN_CONTOUR_AREA) {
            totalMovementArea += area;
        }
    }

    // 如果需要返回运动面积
    if (movementArea != nullptr) {
        *movementArea = totalMovementArea;
    }

    // 根据运动面积判断是否有显著变化
    bool hasChange = (totalMovementArea > MIN_MOVEMENT_AREA);

    // 如果需要显示调试信息
    if (showDebug) {
        visualizeTwoFrameComparison(frame1, frame2, frameDiff, thresholdDiff,
                                    totalMovementArea, hasChange);
    }

    return hasChange;
}

// 独立的测试函数：判断两个图像文件之间是否有变化
inline bool testTwoFrames(const cv::Mat& frame1, const cv::Mat& frame2,
                   bool showResult = true, bool saveResult = false) {

    // 检查输入图像是否有效
    if (frame1.empty() || frame2.empty()) {
        std::cerr << "错误: 输入图像为空!" << std::endl;
        return false;
    }

    std::cout << "开始两帧图像变化检测..." << std::endl;
    std::cout << "帧1: " << frame1.cols << "x" << frame1.rows
              << " 通道: " << frame1.channels() << std::endl;
    std::cout << "帧2: " << frame2.cols << "x" << frame2.rows
              << " 通道: " << frame2.channels() << std::endl;

    double movementArea = 0;
    bool hasChange = hasSignificantChange(frame1, frame2, true, &movementArea);

    std::cout << "检测结果: " << (hasChange ? "有显著变化" : "无显著变化") << std::endl;
    std::cout << "运动区域面积: " << movementArea << " 像素" << std::endl;
    std::cout << "阈值: " << MIN_MOVEMENT_AREA << std::endl;

    // 如果需要保存结果
    if (saveResult && hasChange) {
        saveComparisonResult(frame1, frame2, movementArea, hasChange);
    }

    if (showResult) {
        std::cout << "按任意键继续..." << std::endl;
        cv::waitKey(0);
    }

    return hasChange;
}

#endif // FRAME_CHANGE_DETECTOR_HPP

