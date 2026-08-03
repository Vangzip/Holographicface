#include "multiviewMemoryDump.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <windows.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
double secondsBetween(std::chrono::high_resolution_clock::time_point start,
                      std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

std::string normalizedDirectory(std::string path) {
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }
    while (!path.empty() && (path[path.size() - 1] == '\\' || path[path.size() - 1] == '/')) {
        path.erase(path.size() - 1);
    }
    return path;
}

void createOneDirectory(const std::string& path) {
    if (path.empty()) {
        return;
    }
    if (path.size() == 2 && path[1] == ':') {
        return;
    }
    if (path.size() == 3 && path[1] == ':' && path[2] == '\\') {
        return;
    }
    if (CreateDirectoryA(path.c_str(), NULL)) {
        return;
    }
    const DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS) {
        return;
    }
    std::ostringstream message;
    message << "failed to create output directory: " << path << " error=" << error;
    throw std::runtime_error(message.str());
}

void ensureDirectory(const std::string& directory) {
    const std::string path = normalizedDirectory(directory);
    if (path.empty()) {
        throw std::invalid_argument("output directory is required for memory dump");
    }

    std::size_t start = 0;
    if (path.size() > 2 && path[1] == ':' && path[2] == '\\') {
        start = 3;
    }

    for (std::size_t i = start; i < path.size(); ++i) {
        if (path[i] == '\\') {
            createOneDirectory(path.substr(0, i));
        }
    }
    createOneDirectory(path);
}

std::string frameFileName(const MultiviewFrame& frame) {
    std::ostringstream name;
    name << std::setfill('0') << std::setw(3) << (frame.row + 1)
         << std::setfill('0') << std::setw(3) << (frame.column + 1)
         << ".jpg";
    return name.str();
}

std::string combinePath(const std::string& directory, const std::string& fileName) {
    if (directory.empty()) {
        return fileName;
    }
    const char last = directory[directory.size() - 1];
    if (last == '\\' || last == '/') {
        return directory + fileName;
    }
    return directory + "\\" + fileName;
}
}

MultiviewMemoryDumpStats dumpMultiviewMemoryFrames(const MultiviewRenderPlan& plan,
                                                   const MemoryFrameSink& sink,
                                                   const std::string& outputDirectory) {
    if (sink.data() == nullptr) {
        throw std::logic_error("frame buffer storage was not allocated");
    }

    const auto start = std::chrono::high_resolution_clock::now();
    const std::string directory = normalizedDirectory(outputDirectory);
    ensureDirectory(directory);

    MultiviewMemoryDumpStats stats = {};
    const int resolution = plan.resolution();
    const std::vector<int> jpegParams = {cv::IMWRITE_JPEG_QUALITY, 95};

    for (std::uint64_t index = 0; index < plan.frameCount(); ++index) {
        const MultiviewFrame frame = plan.frameAt(index);
        const unsigned char* frameData = sink.data() + static_cast<std::size_t>(frame.byteOffset);

        cv::Mat rgb(resolution, resolution, CV_8UC3, const_cast<unsigned char*>(frameData));
        cv::Mat flipped;
        cv::Mat bgr;
        cv::flip(rgb, flipped, 0);
        cv::cvtColor(flipped, bgr, cv::COLOR_RGB2BGR);

        const std::string filePath = combinePath(directory, frameFileName(frame));
        try {
            if (cv::imwrite(filePath, bgr, jpegParams)) {
                ++stats.framesWritten;
            } else {
                ++stats.writeErrors;
            }
        } catch (const cv::Exception&) {
            ++stats.writeErrors;
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    stats.seconds = secondsBetween(start, end);
    return stats;
}
