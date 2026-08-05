#include "../V2PrintTiming.h"

#include <QCoreApplication>
#include <QDebug>

#include <cmath>
#include <cstdlib>
#include <limits>

namespace {

void expect(bool condition, const QString& message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        std::exit(1);
    }
}

V2PrintPlan build(const Print9030Config& config, double refreshHz, QString* error)
{
    return buildV2PrintPlan(config, PrintHardwareProfile(), refreshHz, error);
}

void expectInvalid(
    const Print9030Config& config,
    double refreshHz,
    const QString& errorFragment)
{
    QString error = "stale";
    const V2PrintPlan plan = build(config, refreshHz, &error);
    expect(plan.rows.isEmpty(), "invalid input must return an empty row plan");
    expect(error.contains(errorFragment, Qt::CaseInsensitive),
        QString("error '%1' must identify '%2'").arg(error, errorFragment));
}

void testDefaultTwoByThreeGoldenPlan()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 3;
    QString error = "stale";

    const V2PrintPlan plan = build(config, 60.0, &error);

    expect(error.isEmpty(), QString("default plan failed: %1").arg(error));
    expect(plan.stepPulse == 1000, "IMC60G V2 step basis must be 1000");
    expect(plan.accelerationPulse == 12000,
        "V2 acceleration pulse must use rounded trapezoid distance");
    expect(plan.exposurePulse == 3000, "three images require 3000 exposure pulses");
    expect(plan.totalPulse == 43000,
        "total pulse must include exposure, two ramps, and addTemp");
    expect(plan.framesPerImage == 1, "60 kHz at 60 Hz is one VBlank per image");
    expect(plan.presentPredictPulse == 1000,
        "V2's first-image selector must predict one 60 Hz refresh of Y travel");
    expect(plan.rows.size() == 2, "golden plan must contain two rows");

    const V2RowPlan& forward = plan.rows[0];
    expect(forward.row == 0 && !forward.reverse,
        "active start_new begins with bForward=false/reverse=false");
    expect(forward.yStart == 0 && forward.yTarget == 43000,
        "forward row must start at home and move positive");
    expect(forward.constantBegin == 12000, "forward constant segment begins after acceleration");
    expect(forward.exposureBegin == 14000,
        "forward exposure begin includes the +2000 IMC offset");
    expect(forward.compareBegin == 14000 && forward.compareEnd == 17000,
        "IMC60G forward comparison must keep the finite exposure window");
    expect(forward.logicalFrameOrder == QVector<int>({0, 1, 2}),
        "active start_new forward row order must be 0,1,2");
    expect(forward.startDelayFrames == 4,
        "forward display delay must use the 4000-pulse profile constant");
    expect(forward.holdFramesAfterPresent == 0,
        "verified VBlank path holds framesPerImage-1 slots");

    const V2RowPlan& reverse = plan.rows[1];
    expect(reverse.row == 1 && reverse.reverse,
        "active start_new toggles bForward=true/reverse=true for row two");
    expect(reverse.yStart == 43000 && reverse.yTarget == 0,
        "reverse row must return to the homed Y origin");
    expect(reverse.constantBegin == 31000, "reverse constant segment begins after negative ramp");
    expect(reverse.exposureBegin == 18000,
        "reverse exposure begin must include addTemp, one step, and IMC offset");
    expect(reverse.compareBegin == 15000 && reverse.compareEnd == 18000,
        "IMC60G reverse comparison must sort its finite endpoints");
    expect(reverse.compareBegin != std::numeric_limits<qint32>::min()
            && reverse.compareEnd != std::numeric_limits<qint32>::max(),
        "IMC60G must never inherit old-card sentinel windows");
    expect(reverse.logicalFrameOrder == QVector<int>({2, 1, 0}),
        "active start_new reverse row order must be 2,1,0");
    expect(reverse.startDelayFrames == 9,
        "reverse delay must be 16000-2000-4000-1000 = 9000 pulses");
    expect(reverse.holdFramesAfterPresent == 0,
        "one-frame cadence requires no extra hold VBlank");
}

void testProfileConstantsAndReverseDelayBoundaries()
{
    const PrintHardwareProfile profile;
    expect(profile.printStepPulse == 1000, "profile step constant changed");
    expect(profile.forwardDelayPulse == 4000, "profile forward delay changed");
    expect(profile.reverseFixedPulse == 2000, "profile reverse fixed pulse changed");
    expect(profile.exposureOffsetPulse == 2000, "profile exposure offset changed");

    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 1;
    config.main.addTempPulse = 8000;
    config.main.leadPulse = 1000;
    QString error;
    V2PrintPlan plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 1,
        "reverse delay just above zero must round to one VBlank");

    config.main.addTempPulse = 7000;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 0,
        "zero reverse delay must require no initial VBlank");

    config.main.addTempPulse = 5000;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 0,
        "active V2 clamps a negative computed reverse delay to zero frames");

    config.main.addTempPulse = 16000;
    config.main.leadPulse = 0;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 10,
        "lead pulse must be subtracted exactly from reverse display delay");

    config.main.addTempPulse = 16000;
    config.main.leadPulse = -5000;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows[1].startDelayFrames == 15,
        "negative lead pulse must be subtracted as signed input: " + error);
}

void testVBlankFitAndVerifiedOutputHold()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 1;
    QString error;

    V2PrintPlan plan = build(config, 62.9999, &error);
    expect(error.isEmpty() && plan.framesPerImage == 1,
        "frame fit just below V2 0.05 tolerance must pass");

    plan = build(config, 63.0001, &error);
    expect(plan.rows.isEmpty()
            && error.contains("VBlank", Qt::CaseInsensitive),
        "frame fit just above V2 0.05 tolerance must fail");

    config.axisY.speedOfMovement = 30000;
    plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.framesPerImage == 2,
        QString("exact two-VBlank cadence failed: %1").arg(error));
    expect(plan.presentPredictPulse == 500,
        "V2's predictive Y selector must use speed / refreshHz, not image step");
    expect(plan.rows[0].holdFramesAfterPresent == 1,
        "verified physical-output path holds framesPerImage-1");
    expect(plan.rows[0].startDelayFrames == 8,
        "display-delay frames must retain V2 +0.5 integer rounding");

    config.axisY.speedOfMovement = 12000;
    expectInvalid(config, 60.0, "supported");
}

void testEverySupportedMultiVBlankCount()
{
    struct GoldenCadence {
        long speed;
        int framesPerImage;
        int holdFrames;
    };
    const GoldenCadence cases[] = {
        {20000, 3, 2},
        {15000, 4, 3},
        {10000, 6, 5}
    };

    for (const GoldenCadence& golden : cases) {
        Print9030Config config = defaultPrint9030Config();
        config.main.gridRows = 1;
        config.main.gridColumns = 1;
        config.axisY.speedOfMovement = golden.speed;
        QString error;

        const V2PrintPlan plan = build(config, 60.0, &error);

        expect(error.isEmpty(),
            QString("%1-pulse/s cadence failed: %2")
                .arg(golden.speed)
                .arg(error));
        expect(plan.framesPerImage == golden.framesPerImage,
            QString("%1 pulse/s at 60 Hz must require %2 frames per image")
                .arg(golden.speed)
                .arg(golden.framesPerImage));
        expect(plan.rows.size() == 1
                && plan.rows[0].holdFramesAfterPresent == golden.holdFrames,
            QString("%1-frame cadence must hold for %2 additional VBlanks")
                .arg(golden.framesPerImage)
                .arg(golden.holdFrames));
    }
}

void testInvalidConfigurationFailsClosed()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 0;
    expectInvalid(config, 60.0, "grid");

    config = defaultPrint9030Config();
    config.main.gridColumns = -1;
    expectInvalid(config, 60.0, "grid");

    config = defaultPrint9030Config();
    config.main.gridRows = 1001;
    config.main.gridColumns = 3000;
    expectInvalid(config, 60.0, "allocation");

    config = defaultPrint9030Config();
    config.main.widthScale = std::numeric_limits<double>::quiet_NaN();
    expectInvalid(config, 60.0, "widthScale");

    config = defaultPrint9030Config();
    config.axisY.maxDistance = std::numeric_limits<double>::infinity();
    expectInvalid(config, 60.0, "maxDistance");

    config = defaultPrint9030Config();
    config.axisX.maxDistance = std::numeric_limits<double>::quiet_NaN();
    expectInvalid(config, 60.0, "axisX.maxDistance");

    config = defaultPrint9030Config();
    config.axisY.speedOfMovement = 0;
    expectInvalid(config, 60.0, "speed");

    config = defaultPrint9030Config();
    config.axisY.acceleratedVelocity = 0;
    expectInvalid(config, 60.0, "acceleration");

    config = defaultPrint9030Config();
    config.axisY.startSpeed = config.axisY.speedOfMovement + 1;
    expectInvalid(config, 60.0, "startSpeed");

    config = defaultPrint9030Config();
    config.main.columnSpacingMm = 0.4;
    expectInvalid(config, 60.0, "stepPulse");

    config = defaultPrint9030Config();
    config.main.addTempPulse = -1;
    expectInvalid(config, 60.0, "addTempPulse");

    config = defaultPrint9030Config();
    expectInvalid(config, 0.0, "refresh");
    expectInvalid(config, -60.0, "refresh");
    expectInvalid(config, std::numeric_limits<double>::quiet_NaN(), "refresh");
    expectInvalid(config, std::numeric_limits<double>::infinity(), "refresh");
}

void testInactiveLegacyFieldsDoNotAffectV2Plan()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.moveAdjustMm = std::numeric_limits<double>::quiet_NaN();
    config.main.delaySeconds = -100.0;
    config.main.exposureSeconds = 0.0;
    QString error;
    const V2PrintPlan plan = build(config, 60.0, &error);
    expect(error.isEmpty() && !plan.rows.isEmpty(),
        "inactive legacy fields must not gate or alter the active IMC60G V2 path");
}

void testCheckedPositionAndIntermediateArithmetic()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 1;
    config.main.gridColumns = 2147484;
    expectInvalid(config, 60.0, "int32");

    config = defaultPrint9030Config();
    config.axisY.speedOfMovement = 2000000000L;
    config.axisY.acceleratedVelocity = 1;
    expectInvalid(config, 60.0, "int32");

    config = defaultPrint9030Config();
    config.main.gridRows = 2;
    config.main.gridColumns = 1;
    config.main.addTempPulse = 0;
    config.main.leadPulse = std::numeric_limits<long long>::max();
    expectInvalid(config, 60.0, "reverse display delay");

    // This is the only checked qint64 helper overflow reachable through the
    // locked public input types: non-negative int products cannot exceed
    // INT_MAX^2, profile pulse values are fixed, allocation is capped, and
    // acceleration is narrowed to int32 before later checked add/multiply
    // operations. Keep the other checked helpers as defensive guards without
    // claiming direct branch coverage or adding a production test seam.
    config = defaultPrint9030Config();
    config.axisY.subdivision = std::numeric_limits<int>::max();
    config.axisY.resolution = std::numeric_limits<int>::max();
    config.main.columnSpacingMm = 3.0;
    expectInvalid(config, 60.0, "stepPulse");

    // With the locked zero Y anchor and equal positive snake travel, each
    // reverse row returns to zero. Negative qint32 position overflow is not
    // reachable from a valid public configuration; the production lower-bound
    // checks remain intentionally defensive.
}

void testGridProductRepresentsRequiredFrameCount()
{
    Print9030Config config = defaultPrint9030Config();
    config.main.gridRows = 3;
    config.main.gridColumns = 2;
    QString error;
    const V2PrintPlan plan = build(config, 60.0, &error);
    expect(error.isEmpty() && plan.rows.size() == 3,
        "valid non-empty grid must produce one plan per row");
    int plannedFrames = 0;
    for (const V2RowPlan& row : plan.rows) {
        plannedFrames += row.logicalFrameOrder.size();
    }
    expect(plannedFrames == 6,
        "timing plan must represent exactly rows*columns required frames");
    expect(plan.rows[2].yStart == 0
            && plan.rows[2].yTarget == plan.totalPulse,
        "third row must recurse from the prior reverse-row target at Y origin");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testDefaultTwoByThreeGoldenPlan();
    testProfileConstantsAndReverseDelayBoundaries();
    testVBlankFitAndVerifiedOutputHold();
    testEverySupportedMultiVBlankCount();
    testInvalidConfigurationFailsClosed();
    testInactiveLegacyFieldsDoNotAffectV2Plan();
    testCheckedPositionAndIntermediateArithmetic();
    testGridProductRepresentsRequiredFrameCount();
    qInfo() << "V2 print timing tests passed";
    return 0;
}
