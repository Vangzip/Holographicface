#include "multiviewRenderPlan.h"

#include <limits>
#include <stdexcept>

namespace {
std::uint64_t checkedMultiply(std::uint64_t left,
                              std::uint64_t right,
                              const char* message) {
    if (right != 0 &&
        left > std::numeric_limits<std::uint64_t>::max() / right) {
        throw std::length_error(message);
    }
    return left * right;
}
}

MultiviewRenderPlan::MultiviewRenderPlan(int angle, int pre, int resolution)
    : angle_(angle),
      pre_(pre),
      resolution_(resolution),
      samplesPerAxis_(0) {
    if (angle <= 0) {
        throw std::invalid_argument("angle must be positive");
    }
    if (pre <= 0) {
        throw std::invalid_argument("pre must be positive");
    }
    if (resolution <= 0) {
        throw std::invalid_argument("resolution must be positive");
    }

    const std::uint64_t samplesPerAxis =
        checkedMultiply(static_cast<std::uint64_t>(angle),
                        static_cast<std::uint64_t>(pre),
                        "samples per axis overflow");
    if (samplesPerAxis > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("samples per axis exceeds int range");
    }

    const std::uint64_t frameCount =
        checkedMultiply(samplesPerAxis, samplesPerAxis, "frame count overflow");
    const std::uint64_t pixelCount =
        checkedMultiply(static_cast<std::uint64_t>(resolution),
                        static_cast<std::uint64_t>(resolution),
                        "frame pixel count overflow");
    const std::uint64_t frameBytes =
        checkedMultiply(pixelCount, 3, "frame byte count overflow");
    checkedMultiply(frameCount, frameBytes, "total byte count overflow");

    samplesPerAxis_ = static_cast<int>(samplesPerAxis);
}

int MultiviewRenderPlan::angle() const {
    return angle_;
}

int MultiviewRenderPlan::pre() const {
    return pre_;
}

int MultiviewRenderPlan::resolution() const {
    return resolution_;
}

int MultiviewRenderPlan::samplesPerAxis() const {
    return samplesPerAxis_;
}

int MultiviewRenderPlan::channels() const {
    return 3;
}

double MultiviewRenderPlan::stepDegrees() const {
    return 1.0 / static_cast<double>(pre_);
}

std::uint64_t MultiviewRenderPlan::frameCount() const {
    return static_cast<std::uint64_t>(samplesPerAxis_) * static_cast<std::uint64_t>(samplesPerAxis_);
}

std::uint64_t MultiviewRenderPlan::frameBytes() const {
    return static_cast<std::uint64_t>(resolution_) *
           static_cast<std::uint64_t>(resolution_) *
           static_cast<std::uint64_t>(channels());
}

std::uint64_t MultiviewRenderPlan::totalBytes() const {
    return frameCount() * frameBytes();
}

MultiviewFrame MultiviewRenderPlan::frameAt(std::uint64_t index) const {
    if (index >= frameCount()) {
        throw std::out_of_range("frame index out of range");
    }

    const int row = static_cast<int>(index / static_cast<std::uint64_t>(samplesPerAxis_));
    const int column = static_cast<int>(index % static_cast<std::uint64_t>(samplesPerAxis_));
    const double step = stepDegrees();

    MultiviewFrame frame;
    frame.index = index;
    frame.row = row;
    frame.column = column;
    frame.horizontalDegrees = static_cast<double>(column + 1) * step;
    frame.verticalDegrees = static_cast<double>(row + 1) * step;
    frame.byteOffset = index * frameBytes();
    return frame;
}
