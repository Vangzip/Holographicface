#include "../IImc60gApi.h"
#include "../Imc60gMotionController.h"
#include "../PrintConfig.h"
#include "../PrintHardwareProfile.h"
#include "imc60g_safety_tests.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QStringList>

#include <cmath>
#include <cstdlib>

namespace {

constexpr unsigned int kBusy = 0x00000004;
constexpr unsigned int kServoOn = 0x00000002;
constexpr unsigned int kNegativeLimit = 0x00000020;

void expect(bool condition, const QString& message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        std::exit(1);
    }
}

class RecordingImc60gApi final : public IImc60gApi {
public:
    QStringList events;
    QString failAt;
    unsigned int cardCount = 1;
    QSet<short> enabledAxes;
    QSet<short> stoppedAxes;
    QSet<short> servoOffAxes;
    bool ethercatStarted = false;
    bool ethercatStopped = false;
    bool cardOpened = false;
    bool cardClosed = false;
    bool ptpStarted = false;
    unsigned int masterStatus = 6;
    short masterAxisCount = 2;
    short emergency = 0;
    unsigned int forcedAxisBits = 0;
    bool currentPositionWritesUpdatePlanned = false;
    short lastPtpAxis = -1;
    int lastPtpTarget = 0;
    double lastProfileVelocity = 0;
    QHash<short, int> plannedPositions;
    QHash<short, int> returningBusyPolls;
    QHash<short, int> currentPositionSetCounts;
    QHash<short, int> syncPositionCounts;

    int result(const QString& operation) const
    {
        return failAt == operation ? 0x0023 : 0;
    }

    int getCardsNum(unsigned int* count) override
    {
        events << "cards";
        if (result("cards") != 0) {
            return result("cards");
        }
        *count = cardCount;
        return 0;
    }

    int openCard(unsigned int cardIndex) override
    {
        events << QString("open:%1").arg(cardIndex);
        const int rc = result("open");
        cardOpened = rc == 0;
        return rc;
    }

    int closeCard(unsigned int cardIndex) override
    {
        Q_UNUSED(cardIndex);
        cardClosed = true;
        cardOpened = false;
        return result("close");
    }

    int scanEthercat(unsigned int cardIndex, short waitSeconds) override
    {
        events << QString("scan:%1:%2").arg(cardIndex).arg(waitSeconds);
        return result("scan");
    }

    int initEthercat(unsigned int cardIndex) override
    {
        events << QString("ecat_init:%1").arg(cardIndex);
        return result("init");
    }

    int startEthercat(unsigned int cardIndex) override
    {
        events << QString("ecat_start:%1").arg(cardIndex);
        const int rc = result("start");
        ethercatStarted = rc == 0;
        return rc;
    }

    int stopEthercat(unsigned int cardIndex) override
    {
        Q_UNUSED(cardIndex);
        ethercatStopped = true;
        ethercatStarted = false;
        return result("stop_ecat");
    }

    int ethercatMasterStatus(unsigned int cardIndex,
        unsigned int* status) override
    {
        Q_UNUSED(cardIndex);
        if (result("master_status") == 0) *status = masterStatus;
        return result("master_status");
    }

    int ethercatMasterInfo(unsigned int cardIndex,
        Imc60gMasterInfo* info) override
    {
        Q_UNUSED(cardIndex);
        if (result("master_info") == 0 && info) {
            info->axisCount = masterAxisCount;
        }
        return result("master_info");
    }

    int setEmergencyLevel(unsigned int cardIndex, short inverted) override
    {
        events << QString("emg_level:%1:%2").arg(cardIndex).arg(inverted);
        return result("emg");
    }

    int emergencyStatus(unsigned int cardIndex, short* status) override
    {
        Q_UNUSED(cardIndex);
        if (result("emergency_status") == 0) *status = emergency;
        return result("emergency_status");
    }

    int clearAxisStatus(unsigned int cardIndex, short axis) override
    {
        Q_UNUSED(cardIndex);
        if (events.size() < 8) {
            events << QString("clear:%1").arg(axis);
        }
        return result(QString("clear%1").arg(axis));
    }

    int servoOn(unsigned int cardIndex, short axis) override
    {
        Q_UNUSED(cardIndex);
        events << QString("servo_on:%1").arg(axis);
        const int rc = result(QString("servo%1").arg(axis));
        if (rc == 0) {
            enabledAxes.insert(axis);
        }
        return rc;
    }

    int servoOff(unsigned int cardIndex, short axis) override
    {
        Q_UNUSED(cardIndex);
        servoOffAxes.insert(axis);
        enabledAxes.remove(axis);
        return result("servo_off");
    }

    int setMotionProfile(unsigned int cardIndex, short axis, double velocity,
        double acceleration, double deceleration, double startVelocity) override
    {
        Q_UNUSED(cardIndex);
        Q_UNUSED(axis);
        Q_UNUSED(acceleration);
        Q_UNUSED(deceleration);
        Q_UNUSED(startVelocity);
        lastProfileVelocity = velocity;
        return result("profile");
    }

    int setAxisEndVelocity(unsigned int cardIndex, short axis, double endVelocity) override
    {
        Q_UNUSED(cardIndex);
        Q_UNUSED(axis);
        Q_UNUSED(endVelocity);
        return result("end_velocity");
    }

    int startPtp(unsigned int cardIndex, short axis, int target) override
    {
        Q_UNUSED(cardIndex);
        ptpStarted = true;
        lastPtpAxis = axis;
        lastPtpTarget = target;
        if (target == 92000 || target == 28000) {
            events << QString("backoff:%1:%2").arg(axis).arg(target);
            return result(QString("backoff%1").arg(axis));
        }
        if (target == 0 && returningBusyPolls.contains(axis)) {
            returningAxes.insert(axis);
        } else {
            plannedPositions.insert(axis, target);
        }
        return result("ptp");
    }

    int configureJog(unsigned int cardIndex, short axis) override
    {
        Q_UNUSED(cardIndex);
        Q_UNUSED(axis);
        return result("jog_profile");
    }

    int startJogMove(unsigned int cardIndex, short axis, int direction) override
    {
        Q_UNUSED(cardIndex);
        events << QString("home:%1:%2").arg(axis).arg(direction);
        homingAxes.insert(axis);
        return result(QString("home%1").arg(axis));
    }

    int stop(unsigned int cardIndex, short axis, int mode) override
    {
        Q_UNUSED(cardIndex);
        Q_UNUSED(mode);
        stoppedAxes.insert(axis);
        return result("stop");
    }

    int axisStatus(unsigned int cardIndex, short axis, unsigned int* status) override
    {
        Q_UNUSED(cardIndex);
        if (result("axis_status") != 0) {
            return result("axis_status");
        }
        unsigned int bits = (enabledAxes.contains(axis) ? kServoOn : 0)
            | (homingAxes.contains(axis) ? kNegativeLimit : 0)
            | forcedAxisBits;
        if (returningAxes.contains(axis)) {
            int remaining = returningBusyPolls.value(axis);
            if (remaining > 0) {
                returningBusyPolls.insert(axis, remaining - 1);
                bits |= kBusy;
            } else {
                returningAxes.remove(axis);
                plannedPositions.insert(axis, 0);
            }
        }
        *status = bits;
        return 0;
    }

    int stopReason(unsigned int cardIndex, short axis, unsigned int* reason) override
    {
        Q_UNUSED(cardIndex);
        Q_UNUSED(axis);
        *reason = 0;
        return result("stop_reason");
    }

    int plannedPosition(unsigned int cardIndex, short axis, int* position) override
    {
        Q_UNUSED(cardIndex);
        *position = plannedPositions.value(axis);
        return result("planned");
    }

    int encoderPosition(unsigned int cardIndex, short axis, int* position) override
    {
        Q_UNUSED(cardIndex);
        *position = homingAxes.contains(axis) ? 100 : 0;
        return result("encoder");
    }

    int setCurrentPosition(unsigned int cardIndex, short axis, double position) override
    {
        Q_UNUSED(cardIndex);
        ++currentPositionSetCounts[axis];
        if (currentPositionWritesUpdatePlanned) {
            plannedPositions.insert(axis, qRound(position));
        }
        if (!zeroedAxes.contains(axis)) {
            events << QString("zero:%1").arg(axis);
            zeroedAxes.insert(axis);
        }
        return result(QString("zero%1").arg(axis));
    }

    int syncPosition(unsigned int cardIndex, short axis) override
    {
        Q_UNUSED(cardIndex);
        ++syncPositionCounts[axis];
        return result("sync");
    }

    int setAxisSdo(unsigned int, short, unsigned short, unsigned short,
        unsigned char*, unsigned int, unsigned int*) override
    {
        return result("set_sdo");
    }

    int getAxisSdo(unsigned int, short, unsigned short, unsigned short,
        unsigned char*, unsigned int, unsigned int*, unsigned int*) override
    {
        return result("get_sdo");
    }

private:
    QSet<short> homingAxes;
    QSet<short> zeroedAxes;
    QSet<short> returningAxes;
};

class AdvancingClock final : public IImc60gClock {
public:
    qint64 nowMs() const override { return nowMs_; }
    void sleepMs(unsigned long milliseconds) override { nowMs_ += milliseconds; }

private:
    qint64 nowMs_ = 0;
};

void expectSafeShutdown(const RecordingImc60gApi& api)
{
    expect(api.stoppedAxes.contains(0) && api.stoppedAxes.contains(1),
        "failure must stop both configured axes");
    for (short axis : api.servoOffAxes) {
        Q_UNUSED(axis);
    }
    expect(api.enabledAxes.isEmpty(), "failure must servo off every enabled axis");
    expect(api.ethercatStopped, "failure must stop EtherCAT");
    expect(api.cardClosed, "failure must close the card");
}

void testLifecycleAndHomingOrder()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    expect(controller.state() == Imc60gConnectionState::Disconnected, "starts disconnected");
    expect(api.events.isEmpty(), "constructor must not touch hardware");

    QString error;
    expect(controller.connectAndHome(&error), "connect and home should pass with recording API: " + error);
    expect(api.events == QStringList({
        "cards", "open:0", "scan:0:40",
        "emg_level:0:1", "clear:0", "clear:1", "servo_on:0", "servo_on:1",
        "home:0:-1", "backoff:0:92000", "zero:0",
        "home:1:-1", "backoff:1:28000", "zero:1"
    }), "connection and Y/X homing order must match V2");
    expect(controller.state() == Imc60gConnectionState::Ready, "successful homing is ready");
    controller.disconnect();
    expect(controller.state() == Imc60gConnectionState::Disconnected, "disconnect resets state");
}

void testLifecycleFailuresCloseSafely()
{
    const QStringList failurePoints = {
        "scan", "emg", "servo0", "servo1",
        "home0", "backoff0", "zero0", "home1", "backoff1", "zero1"
    };
    for (const QString& point : failurePoints) {
        RecordingImc60gApi api;
        api.failAt = point;
        Imc60gMotionController controller(&api, PrintHardwareProfile());
        QString error;
        expect(!controller.connectAndHome(&error), point + " must fail");
        expect(error.contains("card=0") && error.contains("code=35"),
            point + " error must identify card and numeric vendor code: " + error);
        expect(controller.state() == Imc60gConnectionState::Fault, point + " must leave fault state");
        expectSafeShutdown(api);
    }

    {
        RecordingImc60gApi api;
        api.failAt = "cards";
        Imc60gMotionController controller(&api, PrintHardwareProfile());
        QString error;
        expect(!controller.connectAndHome(&error), "card query error must fail closed");
        expect(error.contains("IMC_GetCardsNum") && error.contains("code=35"),
            "card query error must preserve its numeric return code");
        expect(!api.cardOpened && !api.cardClosed, "card query error must not touch a card");
    }
    {
        RecordingImc60gApi api;
        api.cardCount = 0;
        Imc60gMotionController controller(&api, PrintHardwareProfile());
        QString error;
        expect(!controller.connectAndHome(&error), "zero cards must fail closed");
        expect(!api.cardOpened && !api.cardClosed, "zero cards must not open or close a card");
    }
    {
        RecordingImc60gApi api;
        api.failAt = "open";
        Imc60gMotionController controller(&api, PrintHardwareProfile());
        QString error;
        expect(!controller.connectAndHome(&error), "open failure must fail closed");
        expect(!api.cardClosed, "failed open must not close an unopened card");
    }
}

void testManualMotionValidationAndMapping()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    QString error;
    expect(controller.connectAndHome(&error), "setup connection");
    api.ptpStarted = false;

    PrintAxisConfig axisX;
    axisX.subdivision = 40;
    axisX.resolution = 50;
    axisX.speedOfMovement = 5000;
    axisX.acceleratedVelocity = 50000;
    axisX.startSpeed = 500;
    axisX.stopSpeed = 500;
    axisX.maxDistance = 150.0;
    expect(controller.moveRelative(PrintHardwareProfile::LogicalAxis::X, 10.0, axisX, &error),
        "valid X move should start");
    expect(api.lastPtpAxis == 1 && api.lastPtpTarget == 20000,
        "logical X +10 mm must target physical 1 at 20000 units");

    PrintAxisConfig axisY = axisX;
    expect(controller.moveRelative(PrintHardwareProfile::LogicalAxis::Y, -2.0, axisY, &error),
        "valid Y move should start");
    expect(api.lastPtpAxis == 0 && api.lastPtpTarget == -4000,
        "logical Y must map to physical axis 0");

    const auto expectRejected = [&](PrintHardwareProfile::LogicalAxis logicalAxis,
                                    double distance, const PrintAxisConfig& config,
                                    const QString& label) {
        api.ptpStarted = false;
        expect(!controller.moveRelative(logicalAxis, distance, config, &error), label + " must fail");
        expect(!api.ptpStarted, label + " must not call startPtp");
    };

    expectRejected(PrintHardwareProfile::LogicalAxis::X, 151.0, axisX, "maximum travel");
    PrintAxisConfig invalidSpeed = axisX;
    invalidSpeed.speedOfMovement = 0;
    expectRejected(PrintHardwareProfile::LogicalAxis::X, 1.0, invalidSpeed, "invalid speed");
    expectRejected(PrintHardwareProfile::LogicalAxis::Z, 1.0, axisX, "missing real axis");

    controller.setPrintActive(true);
    expectRejected(PrintHardwareProfile::LogicalAxis::X, 1.0, axisX, "active print");
    controller.setPrintActive(false);
    controller.disconnect();
    expectRejected(PrintHardwareProfile::LogicalAxis::X, 1.0, axisX, "disconnected state");
}

void testSdkOwnershipIsExclusive()
{
    RecordingImc60gApi firstApi;
    RecordingImc60gApi secondApi;
    Imc60gMotionController first(&firstApi, PrintHardwareProfile());
    Imc60gMotionController second(&secondApi, PrintHardwareProfile());
    QString error;
    expect(first.connectAndHome(&error), "first controller should own the SDK");
    expect(!second.connectAndHome(&error), "second controller must not share the SDK card");
    expect(error.contains("already owned"), "ownership failure must be actionable");
    expect(secondApi.events.isEmpty(), "rejected owner must not call the SDK");
    first.disconnect();
    expect(second.connectAndHome(&error), "ownership must be released by disconnect");
}

void testSnapshotsAndExplicitStops()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    QString error;
    expect(controller.connectAndHome(&error), "setup connection");
    Imc60gAxisSnapshot snapshot;
    expect(controller.readSnapshot(PrintHardwareProfile::LogicalAxis::X, &snapshot, &error),
        "ready controller should read a snapshot");
    expect(snapshot.physicalAxis == 1 && snapshot.encoderPosition == 100,
        "snapshot must identify the mapped real axis");
    expect(controller.stopAxis(PrintHardwareProfile::LogicalAxis::Y, &error),
        "explicit stop should succeed");
    expect(api.stoppedAxes.contains(0), "logical Y stop must use physical axis 0");
    expect(controller.isReadyForPrint(), "ready idle controller can print");
    controller.setPrintActive(true);
    expect(!controller.isReadyForPrint(), "active print is not available for another print");
}

void testPrintReadinessUsesLiveHardwareState()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    QString error;
    expect(controller.connectAndHome(&error), "setup connection");

    PrintMotionReadiness readiness = controller.printReadiness(&error);
    expect(readiness.sdkRuntimeReady && readiness.cardReady
            && readiness.ethercatReady && readiness.emergencyClear
            && readiness.servosReady && readiness.axesHomed
            && readiness.axesStopped,
        "connected idle hardware must pass live print readiness: " + error);

    api.enabledAxes.remove(1);
    readiness = controller.printReadiness(&error);
    expect(!readiness.servosReady,
        "Servo Off without an alarm must fail live readiness");
    api.enabledAxes.insert(1);

    api.masterStatus = 3;
    readiness = controller.printReadiness(&error);
    expect(!readiness.ethercatReady,
        "EtherCAT master outside OP must fail live readiness");
    api.masterStatus = 6;

    api.emergency = 1;
    readiness = controller.printReadiness(&error);
    expect(!readiness.emergencyClear,
        "active hardware emergency must fail live readiness");
    api.emergency = 0;

    api.forcedAxisBits = 0x00004000;
    readiness = controller.printReadiness(&error);
    expect(!readiness.ethercatReady && !readiness.servosReady,
        "an unlinked EtherCAT axis must fail live readiness");
    api.forcedAxisBits = 0x00000200;
    readiness = controller.printReadiness(&error);
    expect(!readiness.emergencyClear && !readiness.servosReady,
        "an axis emergency bit must fail live readiness");
    api.forcedAxisBits = kBusy;
    readiness = controller.printReadiness(&error);
    expect(!readiness.axesStopped,
        "a busy axis must fail stopped readiness");
}

void testSlowXReturnUsesItsOwnMotionBudget()
{
    RecordingImc60gApi api;
    AdvancingClock clock;
    api.plannedPositions.insert(1, 149000);
    api.returningBusyPolls.insert(1, 15000);
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString error;
    expect(controller.connectAndHome(&error), "fixture must connect: " + error);
    expect(controller.beginPrint(&error), "fixture must acquire print ownership: " + error);
    PrintAxisConfig axisX;
    axisX.subdivision = 40;
    axisX.resolution = 50;
    axisX.speedOfMovement = 5000;
    axisX.acceleratedVelocity = 50000;
    axisX.startSpeed = 500;
    axisX.stopSpeed = 500;
    axisX.maxDistance = 150.0;
    PrintAxisConfig axisY = axisX;
    axisY.speedOfMovement = 60000;
    axisY.acceleratedVelocity = 150000;
    axisY.startSpeed = 0;
    axisY.stopSpeed = 0;
    expect(controller.returnToLogicalZero(axisX, axisY, 9666, &error),
        "slow X zero return must outlive the Y-derived legacy timeout: " + error);
}

void testReadyStateOriginReturnTemporarilyOwnsPrintMotion()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    QString error;
    expect(controller.connectAndHome(&error), "fixture must connect: " + error);
    api.plannedPositions.insert(1, 12000);
    api.plannedPositions.insert(0, -34000);

    PrintAxisConfig axisX;
    axisX.subdivision = 40;
    axisX.resolution = 50;
    axisX.speedOfMovement = 5000;
    axisX.acceleratedVelocity = 50000;
    axisX.startSpeed = 500;
    axisX.stopSpeed = 500;
    PrintAxisConfig axisY = axisX;

    expect(controller.returnToLogicalZeroWhenReady(axisX, axisY, 5000, &error),
        "ready-state logical origin return must move both axes: " + error);
    expect(api.plannedPositions.value(1) == 0 && api.plannedPositions.value(0) == 0,
        "ready-state logical origin return must target mapped X/Y zero");
    expect(controller.isReadyForPrint(),
        "ready-state logical origin return must release print motion ownership");
}

void testMappedPositionsAndLogicalOrigin()
{
    RecordingImc60gApi api;
    Imc60gMotionController controller(&api, PrintHardwareProfile());
    QString error;
    expect(controller.connectAndHome(&error), "fixture must connect: " + error);

    api.plannedPositions.insert(1, 12000);
    api.plannedPositions.insert(0, -34000);
    api.currentPositionWritesUpdatePlanned = true;
    int xPulses = 0;
    int yPulses = 0;
    expect(controller.readMappedPlannedPositions(&xPulses, &yPulses, &error),
        "mapped planned-position read must succeed: " + error);
    expect(xPulses == 12000 && yPulses == -34000,
        "mapped planned-position read must preserve Axis1/X and Axis0/Y mapping");

    api.stoppedAxes.clear();
    const int xSetBefore = api.currentPositionSetCounts.value(1);
    const int ySetBefore = api.currentPositionSetCounts.value(0);
    const int xSyncBefore = api.syncPositionCounts.value(1);
    const int ySyncBefore = api.syncPositionCounts.value(0);
    expect(controller.setCurrentPositionAsLogicalOrigin(&error),
        "setting logical origin must succeed: " + error);
    expect(api.stoppedAxes.contains(1) && api.stoppedAxes.contains(0),
        "setting logical origin must stop both mapped axes");
    expect(api.currentPositionSetCounts.value(1) == xSetBefore + 2
            && api.currentPositionSetCounts.value(0) == ySetBefore + 2,
        "setting logical origin must zero each mapped axis twice around synchronization");
    expect(api.syncPositionCounts.value(1) == xSyncBefore + 1
            && api.syncPositionCounts.value(0) == ySyncBefore + 1,
        "setting logical origin must synchronize both mapped axes");
    expect(controller.readMappedPlannedPositions(&xPulses, &yPulses, &error)
            && xPulses == 0 && yPulses == 0,
        "setting logical origin must make both mapped positions zero");

    expect(controller.beginPrint(&error), "fixture must acquire print ownership: " + error);
    const int xSetWhilePrinting = api.currentPositionSetCounts.value(1);
    const int ySetWhilePrinting = api.currentPositionSetCounts.value(0);
    expect(!controller.setCurrentPositionAsLogicalOrigin(&error),
        "setting logical origin must be rejected while print ownership is active");
    expect(api.currentPositionSetCounts.value(1) == xSetWhilePrinting
            && api.currentPositionSetCounts.value(0) == ySetWhilePrinting,
        "rejected active-print origin request must not alter either axis coordinate");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (runImc60gSafetyTests(app.arguments())) {
        return 0;
    }
    testLifecycleAndHomingOrder();
    testLifecycleFailuresCloseSafely();
    testManualMotionValidationAndMapping();
    testSdkOwnershipIsExclusive();
    testSnapshotsAndExplicitStops();
    testPrintReadinessUsesLiveHardwareState();
    testSlowXReturnUsesItsOwnMotionBudget();
    testReadyStateOriginReturnTemporarilyOwnsPrintMotion();
    testMappedPositionsAndLogicalOrigin();
    qInfo() << "All IMC60G motion tests passed.";
    return 0;
}
