#include "../IImc60gApi.h"
#include "../PrintHardwareProfile.h"
#include "../Sv660nExposureController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <climits>
#include <cstdlib>
#include <cstring>

namespace {

void expect(bool condition, const QString& message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        std::exit(1);
    }
}

QString eventFor(
    const unsigned char* data, unsigned int size, short axis,
    unsigned short index, unsigned short subIndex)
{
    if (size == sizeof(quint16)) {
        quint16 value = 0;
        std::memcpy(&value, data, sizeof(value));
        return QString("u16:axis%1:%2:%3:%4")
            .arg(axis)
            .arg(index, 4, 16, QLatin1Char('0'))
            .arg(subIndex, 2, 16, QLatin1Char('0'))
            .arg(value)
            .toUpper()
            .replace("AXIS", "axis")
            .replace("U16", "u16");
    }

    qint32 value = 0;
    std::memcpy(&value, data, sizeof(value));
    return QString("s32:axis%1:%2:%3:%4")
        .arg(axis)
        .arg(index, 4, 16, QLatin1Char('0'))
        .arg(subIndex, 2, 16, QLatin1Char('0'))
        .arg(value)
        .toUpper()
        .replace("AXIS", "axis")
        .replace("S32", "s32");
}

class RecordingImc60gApi final : public IImc60gApi {
public:
    QStringList sdoEvents;
    QStringList readbackEvents;
    QStringList timeline;
    QVector<unsigned int> cards;
    QSet<int> failedWriteCalls;
    QSet<int> abortedWriteCalls;
    QSet<int> failedReadCalls;
    QSet<int> abortedReadCalls;
    QSet<int> shortReadCalls;
    QSet<int> mismatchedReadCalls;
    int planned = 1000;
    int plannedRc = 0;
    int plannedCalls = 0;
    unsigned int lastPlannedCard = UINT_MAX;
    short lastPlannedAxis = -1;
    int writeCalls = 0;
    int readCalls = 0;

    int getCardsNum(unsigned int*) override { return 0; }
    int openCard(unsigned int) override { return 0; }
    int closeCard(unsigned int) override { return 0; }
    int scanEthercat(unsigned int, short) override { return 0; }
    int initEthercat(unsigned int) override { return 0; }
    int startEthercat(unsigned int) override { return 0; }
    int stopEthercat(unsigned int) override { return 0; }
    int ethercatMasterStatus(unsigned int, unsigned int* status) override
    {
        if (status) *status = 6;
        return 0;
    }
    int setEmergencyLevel(unsigned int, short) override { return 0; }
    int emergencyStatus(unsigned int, short* status) override
    {
        if (status) *status = 0;
        return 0;
    }
    int clearAxisStatus(unsigned int, short) override { return 0; }
    int servoOn(unsigned int, short) override { return 0; }
    int servoOff(unsigned int, short) override { return 0; }
    int setMotionProfile(
        unsigned int, short, double, double, double, double) override
    {
        return 0;
    }
    int setAxisEndVelocity(unsigned int, short, double) override { return 0; }
    int startPtp(unsigned int, short, int) override { return 0; }
    int configureJog(unsigned int, short) override { return 0; }
    int startJogMove(unsigned int, short, int) override { return 0; }
    int stop(unsigned int, short, int) override { return 0; }
    int axisStatus(unsigned int, short, unsigned int*) override { return 0; }
    int stopReason(unsigned int, short, unsigned int*) override { return 0; }

    int plannedPosition(unsigned int cardIndex, short axis, int* position) override
    {
        ++plannedCalls;
        lastPlannedCard = cardIndex;
        lastPlannedAxis = axis;
        cards << cardIndex;
        timeline << QString("planned:card%1:axis%2").arg(cardIndex).arg(axis);
        if (plannedRc == 0 && position) {
            *position = planned;
        }
        return plannedRc;
    }

    int encoderPosition(unsigned int, short, int*) override { return 0; }
    int setCurrentPosition(unsigned int, short, double) override { return 0; }
    int syncPosition(unsigned int, short) override { return 0; }

    int setAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* abortCode) override
    {
        ++writeCalls;
        const QString event = eventFor(data, size, axis, index, subIndex);
        cards << cardIndex;
        sdoEvents << event;
        timeline << "write:" + event;
        if (abortCode) {
            *abortCode = abortedWriteCalls.contains(writeCalls) ? 0x06090030U : 0U;
        }
        return failedWriteCalls.contains(writeCalls) ? 0x0201 : 0;
    }

    int getAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* resultSize, unsigned int* abortCode) override
    {
        ++readCalls;
        cards << cardIndex;
        if (abortCode) {
            *abortCode = abortedReadCalls.contains(readCalls) ? 0x06020000U : 0U;
        }

        const qint64 value = stableValue(index, subIndex);
        if (size == sizeof(quint16)) {
            quint16 encoded = static_cast<quint16>(value);
            if (mismatchedReadCalls.contains(readCalls)) {
                ++encoded;
            }
            std::memcpy(data, &encoded, sizeof(encoded));
        } else {
            qint32 encoded = static_cast<qint32>(value);
            if (mismatchedReadCalls.contains(readCalls)) {
                ++encoded;
            }
            std::memcpy(data, &encoded, sizeof(encoded));
        }

        if (resultSize) {
            *resultSize = shortReadCalls.contains(readCalls) ? size - 1U : size;
        }
        const QString event = eventFor(data, size, axis, index, subIndex);
        readbackEvents << event;
        timeline << "read:" + event;
        return failedReadCalls.contains(readCalls) ? 0x0201 : 0;
    }

private:
    static qint64 stableValue(unsigned short index, unsigned short subIndex)
    {
        if (index == 0x2004 && subIndex == 0x01) {
            return 25;
        }
        if (index == 0x2018 && subIndex == 0x13) {
            return 256;
        }
        if (index == 0x2018 && subIndex == 0x04) {
            return 0;
        }
        if (index == 0x2018 && subIndex == 0x06) {
            return 1000;
        }
        if (index == 0x2018 && (subIndex == 0x08 || subIndex == 0x09)) {
            return 1;
        }
        if (index == 0x2019 && subIndex == 0x01) {
            return currentTarget_;
        }
        if (index == 0x2019 && subIndex == 0x03) {
            return currentTarget_ >= 0 ? 129 : 130;
        }
        return 0;
    }

public:
    static qint32 currentTarget_;
};

qint32 RecordingImc60gApi::currentTarget_ = 2000;

QStringList positiveWrites()
{
    return {
        "u16:axis0:2004:01:25",
        "u16:axis0:2018:01:0",
        "u16:axis0:2018:13:256",
        "s32:axis0:2018:0D:0",
        "u16:axis0:2018:05:0",
        "u16:axis0:2018:05:1",
        "u16:axis0:2018:05:0",
        "u16:axis0:2018:04:0",
        "u16:axis0:2018:06:1000",
        "u16:axis0:2018:08:1",
        "u16:axis0:2018:09:1",
        "s32:axis0:2019:01:2000",
        "u16:axis0:2019:03:129",
        "u16:axis0:2018:01:1"
    };
}

QStringList positiveReadbacks()
{
    return {
        "u16:axis0:2004:01:25",
        "u16:axis0:2018:13:256",
        "s32:axis0:2018:0D:0",
        "u16:axis0:2018:04:0",
        "u16:axis0:2018:06:1000",
        "u16:axis0:2018:08:1",
        "u16:axis0:2018:09:1",
        "s32:axis0:2019:01:2000",
        "u16:axis0:2019:03:129"
    };
}

void testPositiveSequenceAndWait()
{
    RecordingImc60gApi api;
    RecordingImc60gApi::currentTarget_ = 2000;
    QVector<int> waits;
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [&waits](int milliseconds) {
            waits << milliseconds;
        });
    QString error;

    expect(exposure.arm(3000, LONG_MAX, &error),
        QString("positive arm should succeed: %1").arg(error));
    expect(error.isEmpty(), "successful arm should clear the error");
    expect(exposure.isArmed(), "successful arm must report armed");
    expect(api.plannedCalls == 1 && api.lastPlannedCard == 0
            && api.lastPlannedAxis == 0,
        "exposure must read card 0 physical Y axis 0");
    expect(api.sdoEvents == positiveWrites(),
        QString("positive crossing writes differ:\n%1")
            .arg(api.sdoEvents.join('\n')));
    expect(api.readbackEvents == positiveReadbacks(),
        QString("positive crossing readbacks differ:\n%1")
            .arg(api.readbackEvents.join('\n')));
    expect(waits == QVector<int>({5}), "H18.04 edge must wait exactly 5 ms");
    for (unsigned int card : api.cards) {
        expect(card == 0, "every planned-position and SDO call must use card 0");
    }

    const QString finalEnable = "write:u16:axis0:2018:01:1";
    const int finalEnableIndex = api.timeline.lastIndexOf(finalEnable);
    expect(finalEnableIndex == api.timeline.size() - 1,
        "final H18.00=1 must be the final SDK operation");
    for (const QString& stableReadback : positiveReadbacks()) {
        const int readIndex = api.timeline.indexOf("read:" + stableReadback);
        expect(readIndex >= 0 && readIndex < finalEnableIndex,
            QString("stable readback must precede final enable: %1")
                .arg(stableReadback));
    }
}

void testReverseSequence()
{
    RecordingImc60gApi api;
    api.planned = 9000;
    RecordingImc60gApi::currentTarget_ = -2000;
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [](int) {});
    QString error;

    expect(exposure.arm(LONG_MIN, 7000, &error),
        QString("reverse arm should succeed: %1").arg(error));
    QStringList expected = positiveWrites();
    expected[11] = "s32:axis0:2019:01:-2000";
    expected[12] = "u16:axis0:2019:03:130";
    expect(api.sdoEvents == expected, "reverse target/attribute must match V2");
    QStringList expectedReads = positiveReadbacks();
    expectedReads[7] = "s32:axis0:2019:01:-2000";
    expectedReads[8] = "u16:axis0:2019:03:130";
    expect(api.readbackEvents == expectedReads,
        "reverse target/attribute readback must match V2");
}

void testEveryWriteReturnAndAbortFailureDisarms()
{
    constexpr int kArmWriteCount = 14;
    for (int call = 1; call <= kArmWriteCount; ++call) {
        for (int abortFailure = 0; abortFailure <= 1; ++abortFailure) {
            RecordingImc60gApi api;
            RecordingImc60gApi::currentTarget_ = 2000;
            if (abortFailure) {
                api.abortedWriteCalls.insert(call);
            } else {
                api.failedWriteCalls.insert(call);
            }
            Sv660nExposureController exposure(
                api, PrintHardwareProfile(), [](int) {});
            QString error;
            expect(!exposure.arm(3000, LONG_MAX, &error),
                QString("write failure %1/%2 must fail arm")
                    .arg(call)
                    .arg(abortFailure));
            expect(!exposure.isArmed(), "failed arm must never report armed");
            expect(api.sdoEvents.last() == "u16:axis0:2018:01:0",
                "failed arm must best-effort disable H18.00");
            expect(error.contains("index=0x", Qt::CaseInsensitive)
                    && error.contains("sub=0x", Qt::CaseInsensitive)
                    && error.contains("value=")
                    && error.contains("return=0x", Qt::CaseInsensitive)
                    && error.contains("abort=0x", Qt::CaseInsensitive),
                QString("write error must be actionable: %1").arg(error));
        }
    }
}

void testEveryReadReturnAndAbortFailureDisarms()
{
    constexpr int kReadCount = 9;
    for (int call = 1; call <= kReadCount; ++call) {
        for (int abortFailure = 0; abortFailure <= 1; ++abortFailure) {
            RecordingImc60gApi api;
            RecordingImc60gApi::currentTarget_ = 2000;
            if (abortFailure) {
                api.abortedReadCalls.insert(call);
            } else {
                api.failedReadCalls.insert(call);
            }
            Sv660nExposureController exposure(
                api, PrintHardwareProfile(), [](int) {});
            QString error;
            expect(!exposure.arm(3000, LONG_MAX, &error),
                QString("read failure %1/%2 must fail arm")
                    .arg(call)
                    .arg(abortFailure));
            expect(!exposure.isArmed(), "read failure must not report armed");
            expect(api.sdoEvents.last() == "u16:axis0:2018:01:0",
                "read failure must best-effort disable H18.00");
            expect(error.contains("read", Qt::CaseInsensitive)
                    && error.contains("return=0x", Qt::CaseInsensitive)
                    && error.contains("abort=0x", Qt::CaseInsensitive),
                QString("read error must be actionable: %1").arg(error));
        }
    }
}

void testShortAndMismatchedReadbacks()
{
    for (int readCall : {1, 3, 8}) {
        RecordingImc60gApi api;
        RecordingImc60gApi::currentTarget_ = 2000;
        api.shortReadCalls.insert(readCall);
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            QString("short read %1 must fail").arg(readCall));
        expect(error.contains("size", Qt::CaseInsensitive),
            QString("short read error must name size: %1").arg(error));
        expect(api.sdoEvents.last() == "u16:axis0:2018:01:0",
            "short read must disarm");
    }

    for (int readCall : {1, 3, 8}) {
        RecordingImc60gApi api;
        RecordingImc60gApi::currentTarget_ = 2000;
        api.mismatchedReadCalls.insert(readCall);
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            QString("mismatch read %1 must fail").arg(readCall));
        expect(error.contains("expected", Qt::CaseInsensitive)
                && error.contains("actual", Qt::CaseInsensitive),
            QString("mismatch error must show values: %1").arg(error));
        expect(api.sdoEvents.last() == "u16:axis0:2018:01:0",
            "mismatch read must disarm");
    }
}

void testPlannedPositionFailureAndInvalidInputs()
{
    {
        RecordingImc60gApi api;
        api.plannedRc = 0x033c;
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            "planned-position failure must fail");
        expect(error.contains("plannedPosition")
                && error.contains("0x0000033C", Qt::CaseInsensitive),
            QString("planned-position error must be actionable: %1").arg(error));
        expect(api.sdoEvents == QStringList({"u16:axis0:2018:01:0"}),
            "planned-position failure must best-effort disarm only");
    }

    {
        RecordingImc60gApi api;
        PrintHardwareProfile invalid;
        invalid.axisY = 1;
        Sv660nExposureController exposure(api, invalid, [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            "invalid production profile must fail");
        expect(api.plannedCalls == 0 && api.writeCalls == 0 && api.readCalls == 0,
            "invalid profile must fail before every SDK call");
        expect(exposure.isArmed(),
            "invalid profile must not claim a confirmed safe hardware state");
        expect(error.contains("profile", Qt::CaseInsensitive),
            "invalid profile error must be actionable");
    }

    {
        RecordingImc60gApi api;
        api.planned = INT_MAX;
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(LONG_MIN, LONG_MIN, &error),
            "relative target beyond int32 must fail");
        expect(api.readCalls == 0, "overflow must not read any SDO");
        expect(api.sdoEvents == QStringList({"u16:axis0:2018:01:0"}),
            "overflow must issue only the fail-closed disable");
        expect(error.contains("int32", Qt::CaseInsensitive),
            QString("overflow error must be actionable: %1").arg(error));
        expect(!exposure.isArmed(),
            "successful overflow cleanup must confirm the disabled state");
    }

    {
        RecordingImc60gApi api;
        api.planned = INT_MIN;
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(LONG_MAX, LONG_MAX, &error),
            "positive relative target beyond int32 must fail");
        expect(api.readCalls == 0, "positive overflow must not read any SDO");
        expect(api.sdoEvents == QStringList({"u16:axis0:2018:01:0"}),
            "positive overflow must issue only the fail-closed disable");
        expect(!exposure.isArmed(),
            "successful positive-overflow cleanup must confirm disabled state");
    }
}

void testCleanupFailureIsAggregated()
{
    RecordingImc60gApi api;
    api.failedWriteCalls = {1, 2};
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [](int) {});
    QString error;

    expect(!exposure.arm(3000, LONG_MAX, &error),
        "arm and cleanup failures must fail");
    expect(exposure.isArmed(),
        "failed arm with failed cleanup must remain conservatively unsafe");
    expect(error.contains("cleanup", Qt::CaseInsensitive)
            && error.count("return=0x", Qt::CaseInsensitive) >= 2,
        QString("cleanup failure must be aggregated: %1").arg(error));
}

void testCleanupAbortAndFinalEnableCleanupFailureRemainUnsafe()
{
    {
        RecordingImc60gApi api;
        api.failedWriteCalls.insert(1);
        api.abortedWriteCalls.insert(2);
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            "cleanup abort must fail arm");
        expect(exposure.isArmed(),
            "cleanup abort must retain the conservative unsafe state");
        expect(error.contains("cleanup", Qt::CaseInsensitive)
                && error.contains("abort=0X06090030", Qt::CaseInsensitive),
            QString("cleanup abort must be aggregated: %1").arg(error));
    }

    {
        RecordingImc60gApi api;
        api.failedWriteCalls = {14, 15};
        RecordingImc60gApi::currentTarget_ = 2000;
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(!exposure.arm(3000, LONG_MAX, &error),
            "final enable plus cleanup failure must fail arm");
        expect(exposure.isArmed(),
            "unconfirmed cleanup after final enable attempt must remain unsafe");
        expect(api.sdoEvents[13] == "u16:axis0:2018:01:1"
                && api.sdoEvents[14] == "u16:axis0:2018:01:0",
            "failed final enable must be followed by explicit disable");
    }
}

void testNearestBoundaryNormalizationAndTieBreak()
{
    struct Case {
        int current;
        long begin;
        long end;
        int relativeTarget;
        int attribute;
    };
    const QVector<Case> cases = {
        {5000, 3000, 9000, -2000, 130},
        {8000, 3000, 9000, 1000, 129},
        {6000, 9000, 3000, -3000, 130},
        {4000, 9000, 3000, -1000, 130}
    };

    for (const Case& item : cases) {
        RecordingImc60gApi api;
        api.planned = item.current;
        RecordingImc60gApi::currentTarget_ = item.relativeTarget;
        Sv660nExposureController exposure(
            api, PrintHardwareProfile(), [](int) {});
        QString error;
        expect(exposure.arm(item.begin, item.end, &error),
            QString("nearest-boundary case should arm: %1").arg(error));
        expect(api.sdoEvents[11]
                == QString("s32:axis0:2019:01:%1").arg(item.relativeTarget),
            "nearest boundary must produce the expected relative target");
        expect(api.sdoEvents[12]
                == QString("u16:axis0:2019:03:%1").arg(item.attribute),
            "nearest boundary must select the matching crossing attribute");
    }
}

void testDisarmAndRepeatedDisarm()
{
    RecordingImc60gApi api;
    RecordingImc60gApi::currentTarget_ = 2000;
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [](int) {});
    QString error;
    expect(exposure.arm(3000, LONG_MAX, &error), "setup arm should succeed");
    api.sdoEvents.clear();

    expect(exposure.disarm(&error), "disarm should succeed");
    expect(!exposure.isArmed(), "successful disarm must clear armed state");
    expect(api.sdoEvents == QStringList({"u16:axis0:2018:01:0"}),
        "disarm must only disable H18.00");

    expect(exposure.disarm(&error), "repeated disarm should re-confirm safe state");
    expect(api.sdoEvents == QStringList({
               "u16:axis0:2018:01:0",
               "u16:axis0:2018:01:0"
           }),
        "repeated disarm must write the safe state again");
}

void testDisarmFailureRemainsConservativelyUnsafe()
{
    RecordingImc60gApi api;
    RecordingImc60gApi::currentTarget_ = 2000;
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [](int) {});
    QString error;
    expect(exposure.arm(3000, LONG_MAX, &error), "setup arm should succeed");
    api.failedWriteCalls.insert(api.writeCalls + 1);

    expect(!exposure.disarm(&error), "failed disarm must be observable");
    expect(exposure.isArmed(),
        "failed disarm must conservatively retain unsafe armed state");
    expect(error.contains("H18.00")
            && error.contains("return=0x", Qt::CaseInsensitive),
        QString("disarm failure must be actionable: %1").arg(error));

    api.failedWriteCalls.clear();
    expect(exposure.disarm(&error), "disarm retry should be permitted");
    expect(!exposure.isArmed(), "successful retry must restore safe state");
}

void testStandaloneDisarmAbortRemainsConservativelyUnsafe()
{
    RecordingImc60gApi api;
    RecordingImc60gApi::currentTarget_ = 2000;
    Sv660nExposureController exposure(
        api, PrintHardwareProfile(), [](int) {});
    QString error;
    expect(exposure.arm(3000, LONG_MAX, &error), "setup arm should succeed");
    api.abortedWriteCalls.insert(api.writeCalls + 1);

    expect(!exposure.disarm(&error), "disarm abort must be observable");
    expect(exposure.isArmed(),
        "disarm abort must retain conservative unsafe state");
    expect(error.contains("abort=0X06090030", Qt::CaseInsensitive),
        QString("disarm abort must be actionable: %1").arg(error));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testPositiveSequenceAndWait();
    testReverseSequence();
    testEveryWriteReturnAndAbortFailureDisarms();
    testEveryReadReturnAndAbortFailureDisarms();
    testShortAndMismatchedReadbacks();
    testPlannedPositionFailureAndInvalidInputs();
    testCleanupFailureIsAggregated();
    testCleanupAbortAndFinalEnableCleanupFailureRemainUnsafe();
    testNearestBoundaryNormalizationAndTieBreak();
    testDisarmAndRepeatedDisarm();
    testDisarmFailureRemainsConservativelyUnsafe();
    testStandaloneDisarmAbortRemainsConservativelyUnsafe();
    qInfo() << "SV660N exposure tests passed";
    return 0;
}
