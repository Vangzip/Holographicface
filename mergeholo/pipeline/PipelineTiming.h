#pragma once

#include "PipelineContext.h"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

inline double elapsedSeconds(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

inline std::string formatSeconds(double seconds)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << seconds;
    return out.str();
}

inline std::string formatBytes(size_t bytes)
{
    const char* units[] = { "B", "KiB", "MiB", "GiB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        ++unit;
    }

    std::ostringstream out;
    if (unit == 0) {
        out << static_cast<size_t>(value) << " " << units[unit];
    }
    else {
        out << std::fixed << std::setprecision(2) << value << " " << units[unit];
    }
    return out.str();
}

inline bool checkedMultiply(size_t left, size_t right, size_t& result)
{
    if (left != 0 && right > std::numeric_limits<size_t>::max() / left) {
        return false;
    }

    result = left * right;
    return true;
}

template <typename StageRunner>
int runTimedStage(const std::string& name, StageRunner&& runner, std::vector<StageTiming>& timings)
{
    const auto start = std::chrono::steady_clock::now();
    const int code = runner();
    const double seconds = elapsedSeconds(start);
    timings.push_back({ name, seconds, code });

    std::cout << "[timing] stage " << name << ": " << formatSeconds(seconds)
              << "s, result=" << code << std::endl;
    return code;
}

double totalTimingSeconds(const std::vector<StageTiming>& timings);
void printTimingSummary(const std::vector<StageTiming>& timings);
