#pragma once

#include "PrintConfig.h"

#include <QString>

struct LegacyPrintTiming {
    long yStepPulse = 0;
    long xStepPulse = 0;
    long accelerationPulse = 0;
    long exposurePulse = 0;
    long addTempPulse = 0;
    long totalPulse = 0;
    int framesPerImage = 0;
};

struct LegacyRowPlan {
    long yStart = 0;
    long yTarget = 0;
    long exposureBegin = 0;
    long exposureEnd = 0;
    long compareBegin = 0;
    long compareEnd = 0;
    bool reverse = false;
};

bool calculateLegacyPrintTiming(
    const Print9030Config& config,
    LegacyPrintTiming* timing,
    QString* errorMessage = nullptr);
LegacyRowPlan makeLegacyRowPlan(const LegacyPrintTiming& timing, long yStart, bool reverse);
