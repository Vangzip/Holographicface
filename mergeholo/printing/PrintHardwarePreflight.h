#pragma once

#include "IExposureController.h"
#include "IMotionController.h"
#include "IPrintFramePresenter.h"
#include "PrintConfig.h"
#include "PrintHardwareProfile.h"
#include "PrintImageSource.h"
#include "V2PrintTiming.h"

#include <QString>

enum class PreflightFault {
    None,
    SdkRuntime,
    Card,
    Ethercat,
    AxisMapping,
    Servo,
    Emergency,
    Homing,
    ExposureProfile,
    ExposureBaseline,
    TimingPlan,
    ImageCount,
    ImageFrame,
    SecondScreen,
    Presenter,
    VBlank
};

struct PrintPreflightResult {
    bool ok = false;
    PreflightFault fault = PreflightFault::None;
    QString detail;
};

struct PrintJobSnapshot {
    Print9030Config config;
    PrintHardwareProfile profile;
    V2PrintPlan plan;
    PrintImageSet images;
};

class IPrintHardwarePreflight
{
public:
    virtual ~IPrintHardwarePreflight() = default;
    virtual PrintPreflightResult check(const PrintJobSnapshot& job,
        bool dynamicOnly) = 0;
};

class PrintHardwarePreflight final : public IPrintHardwarePreflight
{
public:
    PrintHardwarePreflight(IMotionController& motion,
        IExposureController& exposure,
        IPrintFramePresenter& presenter);

    PrintPreflightResult check(const PrintJobSnapshot& job,
        bool dynamicOnly) override;

private:
    IMotionController& motion_;
    IExposureController& exposure_;
    IPrintFramePresenter& presenter_;
};
