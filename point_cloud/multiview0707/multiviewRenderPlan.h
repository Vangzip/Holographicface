#ifndef MULTIVIEW_RENDER_PLAN_H
#define MULTIVIEW_RENDER_PLAN_H

#include <cstddef>
#include <cstdint>

struct MultiviewFrame {
    std::uint64_t index;
    int row;
    int column;
    double horizontalDegrees;
    double verticalDegrees;
    std::uint64_t byteOffset;
};

class MultiviewRenderPlan {
public:
    MultiviewRenderPlan(int angle, int pre, int resolution);

    int angle() const;
    int pre() const;
    int resolution() const;
    int samplesPerAxis() const;
    int channels() const;
    double stepDegrees() const;
    std::uint64_t frameCount() const;
    std::uint64_t frameBytes() const;
    std::uint64_t totalBytes() const;
    MultiviewFrame frameAt(std::uint64_t index) const;

private:
    int angle_;
    int pre_;
    int resolution_;
    int samplesPerAxis_;
};

#endif
