#include "imc60g_safety_tests.h"

#include "../IImc60gApi.h"
#include "../Imc60gMotionController.h"
#include "../PrintConfig.h"
#include "../PrintHardwareProfile.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QRegExp>
#include <QSet>
#include <QThread>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <thread>

namespace {

constexpr unsigned int kAlarm = 0x00000001;
constexpr unsigned int kBusy = 0x00000004;
constexpr unsigned int kPositiveLimit = 0x00000010;
constexpr unsigned int kNegativeLimit = 0x00000020;
constexpr int kFailureCode = 0x0161;

void check(bool condition, const QString& message)
{
    if (!condition) {
        const QByteArray utf8 = message.toUtf8();
        std::fprintf(stderr, "SAFETY FAIL: %s\n", utf8.constData());
        std::fflush(stderr);
        std::exit(1);
    }
}

class AdvancingClock final : public IImc60gClock {
public:
    qint64 stepMs = 1;

    qint64 nowMs() const override
    {
        return now_.load();
    }

    void sleepMs(unsigned long milliseconds) override
    {
        Q_UNUSED(milliseconds);
        now_.fetch_add(stepMs);
        std::this_thread::yield();
    }

private:
    std::atomic<qint64> now_ {0};
};

enum class HomeBehavior {
    Limit,
    SeekForever,
    DiStop,
    StableStop,
    WrongLimit,
    AlarmWrongLimit,
    BackoffForever
};

class SafetyApi final : public IImc60gApi {
public:
    QStringList events;
    QStringList cleanupEvents;
    QString failAt;
    QString pollFailAt;
    int failureCode = kFailureCode;
    unsigned int forcedStopReason = 0;
    HomeBehavior behavior = HomeBehavior::Limit;
    unsigned int cardCount = 1;
    QSet<short> homingAxes;
    QSet<short> backoffAxes;
    QSet<short> enabledAxes;
    QSet<short> stopAttempts;
    QSet<short> servoOffAttempts;
    std::atomic<bool> jogStarted {false};
    std::atomic<bool> backoffStarted {false};
    bool shutdownMode = false;
    bool ethercatStopAttempted = false;
    bool closeAttempted = false;
    bool endVelocityCalled = false;
    double endVelocity = -1.0;

    int result(const QString& operation) const
    {
        return failAt == operation || (failAt == "all_cleanup" && shutdownMode)
            ? failureCode : 0;
    }

    void record(const QString& event)
    {
        events << event;
    }

    int getCardsNum(unsigned int* count) override
    {
        record("cards");
        if (result("cards") == 0) {
            *count = cardCount;
        }
        return result("cards");
    }

    int openCard(unsigned int card) override
    {
        record(QString("open:%1").arg(card));
        return result("open");
    }

    int closeCard(unsigned int card) override
    {
        record(QString("close:%1").arg(card));
        closeAttempted = true;
        if (shutdownMode) {
            cleanupEvents << "close";
        }
        return result(shutdownMode ? "cleanup_close" : "close");
    }

    int scanEthercat(unsigned int card, short wait) override
    {
        record(QString("scan:%1:%2").arg(card).arg(wait));
        return result("scan");
    }

    int initEthercat(unsigned int card) override
    {
        record(QString("init:%1").arg(card));
        return result("init");
    }

    int startEthercat(unsigned int card) override
    {
        record(QString("start:%1").arg(card));
        return result("start");
    }

    int stopEthercat(unsigned int card) override
    {
        record(QString("stop_ecat:%1").arg(card));
        ethercatStopAttempted = true;
        if (shutdownMode) {
            cleanupEvents << "stop_ecat";
        }
        return result(shutdownMode ? "cleanup_stop_ecat" : "stop_ecat");
    }

    int ethercatMasterStatus(unsigned int card, unsigned int* status) override
    {
        Q_UNUSED(card);
        if (status) *status = 6;
        return 0;
    }

    int setEmergencyLevel(unsigned int card, short inverted) override
    {
        record(QString("emg:%1:%2").arg(card).arg(inverted));
        return result("emg");
    }

    int emergencyStatus(unsigned int card, short* status) override
    {
        Q_UNUSED(card);
        if (status) *status = 0;
        return 0;
    }

    int clearAxisStatus(unsigned int card, short axis) override
    {
        Q_UNUSED(card);
        record(QString("clear:%1").arg(axis));
        return result(QString("clear%1").arg(axis));
    }

    int servoOn(unsigned int card, short axis) override
    {
        Q_UNUSED(card);
        record(QString("servo_on:%1").arg(axis));
        const int rc = result(QString("servo_on%1").arg(axis));
        if (rc == 0) {
            enabledAxes.insert(axis);
        }
        return rc;
    }

    int servoOff(unsigned int card, short axis) override
    {
        Q_UNUSED(card);
        record(QString("servo_off:%1").arg(axis));
        servoOffAttempts.insert(axis);
        if (shutdownMode) {
            cleanupEvents << QString("servo_off:%1").arg(axis);
        }
        const int rc = result(QString("cleanup_servo_off%1").arg(axis));
        if (rc == 0) {
            enabledAxes.remove(axis);
        }
        return rc;
    }

    int setMotionProfile(unsigned int card, short axis, double velocity,
        double acceleration, double deceleration, double startVelocity) override
    {
        Q_UNUSED(card);
        Q_UNUSED(velocity);
        Q_UNUSED(acceleration);
        Q_UNUSED(deceleration);
        Q_UNUSED(startVelocity);
        record(QString("profile:%1").arg(axis));
        return result(QString("profile%1").arg(axis));
    }

    int setAxisEndVelocity(unsigned int card, short axis, double velocity) override
    {
        Q_UNUSED(card);
        record(QString("end_velocity:%1:%2").arg(axis).arg(velocity));
        endVelocityCalled = true;
        endVelocity = velocity;
        return result("end_velocity");
    }

    int startPtp(unsigned int card, short axis, int target) override
    {
        Q_UNUSED(card);
        record(QString("ptp:%1:%2").arg(axis).arg(target));
        if (target == 92000 || target == 28000) {
            backoffAxes.insert(axis);
            backoffStarted.store(true);
        }
        return result(QString("backoff%1").arg(axis));
    }

    int configureJog(unsigned int card, short axis) override
    {
        Q_UNUSED(card);
        record(QString("jog_profile:%1").arg(axis));
        return result(QString("jog_profile%1").arg(axis));
    }

    int startJogMove(unsigned int card, short axis, int direction) override
    {
        Q_UNUSED(card);
        record(QString("jog_start:%1:%2").arg(axis).arg(direction));
        homingAxes.insert(axis);
        jogStarted.store(true);
        return result(QString("jog_start%1").arg(axis));
    }

    int stop(unsigned int card, short axis, int mode) override
    {
        Q_UNUSED(card);
        Q_UNUSED(mode);
        record(QString("stop:%1").arg(axis));
        stopAttempts.insert(axis);
        if (shutdownMode) {
            cleanupEvents << QString("stop:%1").arg(axis);
        }
        return result(shutdownMode ? QString("cleanup_stop%1").arg(axis) : "stop");
    }

    int axisStatus(unsigned int card, short axis, unsigned int* status) override
    {
        Q_UNUSED(card);
        record(QString("status:%1").arg(axis));
        if (result("axis_status") != 0
            || (pollFailAt == "axis_status" && homingAxes.contains(axis))) {
            return failureCode;
        }
        if (!homingAxes.contains(axis)) {
            *status = 0;
            return 0;
        }
        if (backoffAxes.contains(axis)) {
            *status = behavior == HomeBehavior::BackoffForever ? kBusy : 0;
            return 0;
        }
        switch (behavior) {
        case HomeBehavior::Limit:
        case HomeBehavior::DiStop:
            *status = behavior == HomeBehavior::Limit ? kNegativeLimit : kBusy;
            break;
        case HomeBehavior::SeekForever:
            *status = kBusy;
            break;
        case HomeBehavior::StableStop:
            *status = 0;
            break;
        case HomeBehavior::WrongLimit:
            *status = kPositiveLimit | kBusy;
            break;
        case HomeBehavior::AlarmWrongLimit:
            *status = kAlarm | kPositiveLimit;
            break;
        case HomeBehavior::BackoffForever:
            *status = kNegativeLimit;
            break;
        }
        return 0;
    }

    int stopReason(unsigned int card, short axis, unsigned int* reason) override
    {
        Q_UNUSED(card);
        Q_UNUSED(axis);
        record("stop_reason");
        if (result("stop_reason") != 0
            || (pollFailAt == "stop_reason" && homingAxes.contains(axis))) {
            return failureCode;
        }
        *reason = homingAxes.contains(axis) && forcedStopReason != 0
            ? forcedStopReason
            : behavior == HomeBehavior::DiStop && homingAxes.contains(axis) ? 0x0b : 0;
        return 0;
    }

    int plannedPosition(unsigned int card, short axis, int* position) override
    {
        Q_UNUSED(card);
        Q_UNUSED(axis);
        record("planned");
        if (result("planned") != 0
            || (pollFailAt == "planned" && homingAxes.contains(axis))) {
            return failureCode;
        }
        *position = 0;
        return 0;
    }

    int encoderPosition(unsigned int card, short axis, int* position) override
    {
        Q_UNUSED(card);
        record("encoder");
        if (result("encoder") != 0
            || (pollFailAt == "encoder" && homingAxes.contains(axis))) {
            return failureCode;
        }
        if (behavior == HomeBehavior::WrongLimit) {
            *position = 0;
        } else {
            *position = homingAxes.contains(axis) ? 100 : 0;
        }
        return 0;
    }

    int setCurrentPosition(unsigned int card, short axis, double position) override
    {
        Q_UNUSED(card);
        Q_UNUSED(axis);
        Q_UNUSED(position);
        record("zero");
        return result("zero");
    }

    int syncPosition(unsigned int card, short axis) override
    {
        Q_UNUSED(card);
        Q_UNUSED(axis);
        record("sync");
        return result("sync");
    }

    int setAxisSdo(unsigned int, short, unsigned short, unsigned short,
        unsigned char*, unsigned int, unsigned int*) override
    {
        return 0;
    }

    int getAxisSdo(unsigned int, short, unsigned short, unsigned short,
        unsigned char*, unsigned int, unsigned int*, unsigned int*) override
    {
        return 0;
    }
};

void expectCleanupAttempted(const SafetyApi& api)
{
    check(api.stopAttempts.contains(0) && api.stopAttempts.contains(1),
        "both axes must have stop attempts");
    check(api.servoOffAttempts.contains(0) && api.servoOffAttempts.contains(1),
        "both enabled axes must have Servo Off attempts");
    check(api.ethercatStopAttempted, "EtherCAT delete must be attempted");
    check(api.closeAttempted, "card close must be attempted");
}

void testProfileValidationBeforeSdk()
{
    const auto reject = [](PrintHardwareProfile invalid, const QString& label) {
        SafetyApi api;
        AdvancingClock clock;
        Imc60gMotionController controller(&api, invalid, &clock);
        QString error;
        check(!controller.connectAndHome(&error), label + " profile must fail");
        check(api.events.isEmpty(), label + " profile must make zero API calls");
        check(!error.isEmpty(), label + " profile must return validation error");
    };

    PrintHardwareProfile profile;
    profile.version++;
    reject(profile, "version");
    profile = PrintHardwareProfile();
    profile.homeDirectionY = 1;
    reject(profile, "direction");
    profile = PrintHardwareProfile();
    profile.homeSpeed++;
    reject(profile, "speed");
    profile = PrintHardwareProfile();
    profile.homeTimeoutMs--;
    reject(profile, "timeout");
    profile = PrintHardwareProfile();
    profile.homeBackoffY++;
    reject(profile, "backoff");
    profile = PrintHardwareProfile();
    profile.printStepPulse++;
    reject(profile, "print constant");
    profile = PrintHardwareProfile();
    profile.sv660nWidth++;
    reject(profile, "SDO constant");

    SafetyApi invalidApi;
    SafetyApi validApi;
    AdvancingClock clock;
    profile = PrintHardwareProfile();
    profile.homeBackoffX++;
    Imc60gMotionController invalidController(&invalidApi, profile, &clock);
    QString error;
    check(!invalidController.connectAndHome(&error), "invalid profile must be rejected");
    Imc60gMotionController validController(
        &validApi, PrintHardwareProfile(), &clock);
    check(validController.connectAndHome(&error),
        "invalid profile must not acquire SDK ownership: " + error);
    check(validController.disconnect(&error),
        "valid controller cleanup after ownership check must pass");
}

void testManualStopVelocityAndExactErrors()
{
    {
        SafetyApi api;
        AdvancingClock clock;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(controller.connectAndHome(&error), "manual setup must connect");
        PrintAxisConfig axis;
        axis.subdivision = 40;
        axis.resolution = 50;
        axis.speedOfMovement = 5000;
        axis.acceleratedVelocity = 50000;
        axis.startSpeed = 500;
        axis.stopSpeed = 500;
        axis.maxDistance = 150.0;
        check(controller.moveRelative(PrintHardwareProfile::LogicalAxis::X, 1.0, axis, &error),
            "manual move must pass");
        check(api.endVelocityCalled && api.endVelocity == axis.stopSpeed,
            "manual stopSpeed must reach the end-velocity seam");
        check(api.events.indexOf("profile:1") < api.events.indexOf(
            QString("end_velocity:1:%1").arg(axis.stopSpeed)),
            "end velocity must be configured after the motion profile");
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(controller.connectAndHome(&error), "end velocity failure setup must connect");
        PrintAxisConfig axis;
        axis.subdivision = 40;
        axis.resolution = 50;
        axis.speedOfMovement = 5000;
        axis.acceleratedVelocity = 50000;
        axis.startSpeed = 500;
        axis.stopSpeed = 500;
        axis.maxDistance = 150.0;
        const int ptpCountBefore = api.events.filter(QRegExp("^ptp:")).size();
        api.failAt = "end_velocity";
        check(!controller.moveRelative(
            PrintHardwareProfile::LogicalAxis::X, 1.0, axis, &error),
            "end velocity failure must propagate");
        check(error.contains("IMC_SetAxEndVel") && error.contains("code=353"),
            "end velocity failure must name exact operation/code: " + error);
        check(api.events.filter(QRegExp("^ptp:")).size() == ptpCountBefore,
            "end velocity failure must prevent manual PTP");
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        api.failAt = "jog_profile0";
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), "JogPrf failure must fail");
        check(error.contains("IMC_JogPrf") && !error.contains("IMC_StartJogMove")
                && error.contains("code=353") && error.contains("ERR_ECAT_AX_CNT_OUTRANG"),
            "JogPrf error must preserve exact operation/code/text: " + error);
        expectCleanupAttempted(api);
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        api.failAt = "jog_start0";
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), "StartJogMove failure must fail");
        check(error.contains("IMC_StartJogMove") && error.contains("code=353"),
            "StartJogMove error must name the exact operation: " + error);
        expectCleanupAttempted(api);
    }
}

void testAcceptedAndRejectedHomingStates()
{
    for (HomeBehavior accepted : {HomeBehavior::DiStop, HomeBehavior::StableStop}) {
        SafetyApi api;
        AdvancingClock clock;
        clock.stepMs = accepted == HomeBehavior::StableStop ? 500 : 1;
        api.behavior = accepted;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(controller.connectAndHome(&error), "approved homing state must be accepted: " + error);
        check(controller.disconnect(&error), "accepted homing cleanup must pass");
    }

    for (HomeBehavior rejected : {HomeBehavior::WrongLimit, HomeBehavior::AlarmWrongLimit}) {
        SafetyApi api;
        AdvancingClock clock;
        clock.stepMs = 200000;
        api.behavior = rejected;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), "wrong-limit state must fail");
        if (rejected == HomeBehavior::AlarmWrongLimit) {
            check(error.contains("alarm"), "alarm/wrong-limit must report alarm: " + error);
        } else {
            check(error.contains("timed out"), "wrong limit alone must time out: " + error);
        }
        expectCleanupAttempted(api);
    }
}

void testPollingAndTimeoutFailures()
{
    {
        SafetyApi api;
        AdvancingClock clock;
        clock.stepMs = 200000;
        api.behavior = HomeBehavior::SeekForever;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error) && error.contains("timed out"),
            "seek timeout must fail closed: " + error);
        expectCleanupAttempted(api);
    }

    const struct {
        const char* point;
        const char* operation;
    } pollFailures[] = {
        {"axis_status", "IMC_GetAxSts"},
        {"stop_reason", "IMC_GetAxStopReason"},
        {"planned", "IMC_GetAxPrfPos32"},
        {"encoder", "IMC_GetAxEncPos32"}
    };
    for (const auto& failure : pollFailures) {
        SafetyApi api;
        AdvancingClock clock;
        api.pollFailAt = failure.point;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), QString(failure.point) + " must fail");
        check(error.contains(failure.operation) && error.contains("code=353"),
            QString(failure.point) + " must preserve operation/code: " + error);
        check(api.jogStarted.load(), QString(failure.point) + " must fail during polling");
        expectCleanupAttempted(api);
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        clock.stepMs = 40000;
        api.behavior = HomeBehavior::BackoffForever;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error) && error.contains("backoff timed out"),
            "backoff timeout must fail closed: " + error);
        expectCleanupAttempted(api);
    }
}

void testFailedServoOnIsStillPoweredOff()
{
    SafetyApi api;
    AdvancingClock clock;
    api.failAt = "servo_on1";
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString error;
    check(!controller.connectAndHome(&error), "failed Servo On must fail connection");
    check(api.servoOffAttempts.contains(0) && api.servoOffAttempts.contains(1),
        "successful and unverified Servo On axes must both receive Servo Off");
}

void checkMappedError(
    const QString& error, const QString& operation, const QString& symbol,
    int code, const QString& label)
{
    check(error.contains(operation), label + " must name operation: " + error);
    check(error.contains(symbol), label + " must name errorcode.h symbol: " + error);
    check(error.contains(QString("code=%1").arg(code))
            && error.contains(QString("[0x%1]").arg(static_cast<unsigned int>(code), 0, 16)),
        label + " must show decimal and hexadecimal values: " + error);
    check(error.contains(QString::fromUtf8(u8"\u64CD\u4F5C"))
            && error.contains("Action"),
        label + " must include actionable Chinese/English guidance: " + error);
}

void testTaskErrorCodeDescriptions()
{
    {
        SafetyApi api;
        AdvancingClock clock;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(controller.connectAndHome(&error), "end velocity mapping setup");
        PrintAxisConfig axis;
        axis.subdivision = 40;
        axis.resolution = 50;
        axis.speedOfMovement = 5000;
        axis.acceleratedVelocity = 50000;
        axis.startSpeed = 500;
        axis.stopSpeed = 500;
        axis.maxDistance = 150.0;
        api.failAt = "end_velocity";
        api.failureCode = 0x0104;
        check(!controller.moveRelative(
            PrintHardwareProfile::LogicalAxis::X, 1.0, axis, &error),
            "ERR_ENDVEL_OUTRANG must fail");
        checkMappedError(error, "IMC_SetAxEndVel", "ERR_ENDVEL_OUTRANG",
            0x0104, "end velocity");
    }

    const struct {
        const char* point;
        int code;
        const char* operation;
        const char* symbol;
        const char* label;
    } connectionErrors[] = {
        {"scan", 0x10018201, "IMC_ScanCardEcat", "ERR_ECAT_MASTER_NOT_OP_STS", "wrapped EtherCAT master status"},
        {"emg", 0x0310, "IMC_SetEmgTrigLevelInv", "ERR_HW_ESTP_IS_TRIG", "emergency stop"},
        {"backoff0", 0x033c, "IMC_StartPtpMove", "ERR_AX_BUSY", "axis busy"},
        {"jog_profile0", 0x033a, "IMC_JogPrf", "ERR_AX_SVOFF", "servo off"},
        {"jog_start0", 0x0161, "IMC_StartJogMove", "ERR_ECAT_AX_CNT_OUTRANG", "axis count"}
    };
    for (const auto& item : connectionErrors) {
        SafetyApi api;
        AdvancingClock clock;
        api.failAt = item.point;
        api.failureCode = item.code;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), QString(item.label) + " must fail");
        checkMappedError(error, item.operation, item.symbol, item.code, item.label);
        if (QString(item.point) == "scan" || QString(item.point) == "emg") {
            check(api.stopAttempts.contains(0) && api.stopAttempts.contains(1),
                "pre-servo failure must still stop both axes");
            check(api.servoOffAttempts.isEmpty(),
                "pre-servo failure must not power off axes that were never enabled");
            check(api.ethercatStopAttempted && api.closeAttempted,
                "pre-servo failure must release EtherCAT and card resources");
        } else {
            expectCleanupAttempted(api);
        }
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        api.failAt = "axis_status";
        api.failureCode = 0x7abc;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error), "unknown code must fail");
        check(error.contains("code=31420") && error.contains("[0x7abc]")
                && error.contains("UNRECOGNIZED_IMC_ERROR")
                && error.contains(QString::fromUtf8(u8"\u672A\u8BC6\u522B")),
            "unknown code must preserve value and mark unrecognized: " + error);
        expectCleanupAttempted(api);
    }
}

void testDirectionalLimitStopReasons()
{
    {
        SafetyApi api;
        AdvancingClock clock;
        api.behavior = HomeBehavior::SeekForever;
        api.forcedStopReason = 0x05;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(controller.connectAndHome(&error),
            "negative home must accept stop reason 0x05: " + error);
        check(controller.disconnect(&error), "negative stop reason cleanup");
    }

    {
        SafetyApi api;
        AdvancingClock clock;
        clock.stepMs = 200000;
        api.behavior = HomeBehavior::SeekForever;
        api.forcedStopReason = 0x04;
        Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
        QString error;
        check(!controller.connectAndHome(&error),
            "negative home must reject positive-limit stop reason 0x04");
        check(error.contains("timed out") && error.contains("stop-reason=4"),
            "wrong stop reason must remain diagnostic: " + error);
        expectCleanupAttempted(api);
    }
}

void waitFor(const std::atomic<bool>& flag, const QString& label)
{
    for (int i = 0; i < 5000 && !flag.load(); ++i) {
        QThread::msleep(1);
    }
    check(flag.load(), label + " was not reached");
}

void testCancellation(HomeBehavior behavior, const QString& label)
{
    SafetyApi api;
    AdvancingClock clock;
    api.behavior = behavior;
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString connectError;
    bool connectResult = true;
    std::thread worker([&] {
        connectResult = controller.connectAndHome(&connectError);
    });

    if (behavior == HomeBehavior::SeekForever) {
        waitFor(api.jogStarted, label);
    } else {
        waitFor(api.backoffStarted, label);
    }
    api.shutdownMode = true;
    QString disconnectError;
    const bool disconnected = controller.disconnect(&disconnectError);
    worker.join();
    check(!connectResult && connectError.contains("cancelled"),
        label + " connect must return cancelled: " + connectError);
    check(disconnected, label + " safe cleanup must allow disconnect");
    check(controller.state() == Imc60gConnectionState::Disconnected,
        label + " safe cancellation must end disconnected");
    expectCleanupAttempted(api);
}

void testCancellationPaths()
{
    testCancellation(HomeBehavior::SeekForever, "seek cancellation");
    testCancellation(HomeBehavior::BackoffForever, "backoff cancellation");
}

void runCleanupChild(const QString& failurePoint)
{
    SafetyApi api;
    AdvancingClock clock;
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString error;
    check(controller.connectAndHome(&error), "cleanup child must connect: " + error);
    api.shutdownMode = true;
    api.failAt = failurePoint;
    check(!controller.disconnect(&error), failurePoint + " shutdown must fail");
    check(controller.state() == Imc60gConnectionState::Fault,
        failurePoint + " shutdown must remain Fault");
    check(error.contains("code=353"), failurePoint + " must preserve numeric code: " + error);
    const QString expectedOperation =
        failurePoint == "cleanup_stop_ecat" ? "IMC_DelEcatComm"
        : failurePoint.startsWith("cleanup_stop") ? "IMC_StopMove"
        : failurePoint.startsWith("cleanup_servo_off") ? "IMC_ServoOff"
        : failurePoint == "cleanup_close" ? "IMC_CloseCard"
        : QString();
    if (!expectedOperation.isEmpty()) {
        check(error.contains(expectedOperation),
            failurePoint + " must name exact operation: " + error);
    }
    expectCleanupAttempted(api);

    if (failurePoint == "all_cleanup") {
        check(error.contains("IMC_StopMove") && error.contains("IMC_ServoOff")
                && error.contains("IMC_DelEcatComm") && error.contains("IMC_CloseCard"),
            "aggregate shutdown error must contain every failed operation: " + error);
    }

    SafetyApi secondApi;
    Imc60gMotionController second(&secondApi, PrintHardwareProfile(), &clock);
    QString secondError;
    check(!second.connectAndHome(&secondError), "poisoned SDK must block reacquisition");
    check(secondError.contains("unverified shutdown") && secondApi.events.isEmpty(),
        "poisoned reacquisition must make zero API calls: " + secondError);
}

void testSuccessfulCleanupOrder()
{
    SafetyApi api;
    AdvancingClock clock;
    Imc60gMotionController controller(&api, PrintHardwareProfile(), &clock);
    QString error;
    check(controller.connectAndHome(&error), "cleanup order setup must connect");
    api.shutdownMode = true;
    check(controller.disconnect(&error), "full cleanup must pass: " + error);
    check(api.cleanupEvents == QStringList({
        "stop:0", "stop:1", "servo_off:0", "servo_off:1", "stop_ecat", "close"
    }), "successful cleanup order must match V2 shutdown");
    check(controller.state() == Imc60gConnectionState::Disconnected,
        "successful cleanup may report disconnected");
}

void testCleanupFailuresInChildren()
{
    const QStringList points = {
        "cleanup_stop0", "cleanup_stop1", "cleanup_servo_off0",
        "cleanup_servo_off1", "cleanup_stop_ecat", "cleanup_close", "all_cleanup"
    };
    for (const QString& point : points) {
        QProcess child;
        child.setProgram(QCoreApplication::applicationFilePath());
        child.setArguments({"--cleanup-case", point});
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start();
        check(child.waitForStarted(5000), "cleanup child must start: " + point);
        check(child.waitForFinished(30000), "cleanup child must finish: " + point);
        check(child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0,
            point + " child failed:\n" + child.readAll());
    }
}

} // namespace

bool runImc60gSafetyTests(const QStringList& arguments)
{
    const int childIndex = arguments.indexOf("--cleanup-case");
    if (childIndex >= 0) {
        check(childIndex + 1 < arguments.size(), "cleanup child point is missing");
        runCleanupChild(arguments[childIndex + 1]);
        return true;
    }

    testProfileValidationBeforeSdk();
    testManualStopVelocityAndExactErrors();
    testAcceptedAndRejectedHomingStates();
    testPollingAndTimeoutFailures();
    testFailedServoOnIsStillPoweredOff();
    testTaskErrorCodeDescriptions();
    testDirectionalLimitStopReasons();
    testCancellationPaths();
    testSuccessfulCleanupOrder();
    testCleanupFailuresInChildren();
    return false;
}
