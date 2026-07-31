#include "Imc60gMotionController.h"

#include "IImc60gApi.h"

#include <QMutexLocker>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace {

constexpr unsigned int kAxisAlarm = 0x00000001;
constexpr unsigned int kAxisServoOn = 0x00000002;
constexpr unsigned int kAxisBusy = 0x00000004;
constexpr unsigned int kPositiveLimit = 0x00000010;
constexpr unsigned int kNegativeLimit = 0x00000020;
constexpr unsigned int kAxisEmergency = 0x00000200;
constexpr unsigned int kAxisUnlinked = 0x00004000;
constexpr unsigned int kEthercatMasterOperational = 6;
constexpr short kHardwareEmergencyStop = 0x0001;
constexpr unsigned int kPositiveLimitStopReason = 0x04;
constexpr unsigned int kNegativeLimitStopReason = 0x05;
constexpr unsigned int kDiStopReason = 0x0b;
constexpr short kNoAxis = -1;

QMutex gSdkOwnerMutex;
Imc60gMotionController* gSdkOwner = nullptr;
bool gSdkShutdownPoisoned = false;
QString gSdkShutdownPoisonReason;

class SystemImc60gClock final : public IImc60gClock {
public:
    qint64 nowMs() const override
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - origin_).count();
    }

    void sleepMs(unsigned long milliseconds) override
    {
        QThread::msleep(milliseconds);
    }

private:
    const std::chrono::steady_clock::time_point origin_ =
        std::chrono::steady_clock::now();
};

void setError(QString* destination, const QString& message)
{
    if (destination) {
        *destination = message;
    }
}

void appendTaskError(QString* destination, const QString& message)
{
    if (message.isEmpty()) return;
    if (!destination->isEmpty()) *destination += "; ";
    *destination += message;
}

enum class ErrorAction {
    Transport,
    Parameter,
    Card,
    Ethercat,
    Motion,
    Safety,
    AxisState,
    EndVelocity,
    EmergencyStop,
    ServoOff,
    Busy
};

struct ErrorInfo {
    int code;
    const char* symbol;
    const char* explanation;
    ErrorAction action;
};

// Unique numeric values only. errorcode.h defines ERR_STVEL_OUTRANG as an
// alias of ERR_ENDVEL_OUTRANG (0x0104), so it is named in the same entry.
const ErrorInfo kTaskErrorInfo[] = {
    {0x0001, "ERR_TRANSMIT", "command transmission error", ErrorAction::Transport},
    {0x0002, "ERR_UNKNOWN", "unsupported command", ErrorAction::Transport},
    {0x0003, "ERR_PARSE", "command parse error", ErrorAction::Transport},
    {0x0006, "ERR_CMD_EXECUTE", "command execution error", ErrorAction::Transport},
    {0x0009, "ERR_INPUT", "invalid pointer parameter", ErrorAction::Parameter},
    {0x0020, "ERR_INDEX_OUTRANG", "index out of range", ErrorAction::Parameter},
    {0x0021, "ERR_CARD_INDEX_OUTRANG", "card index out of range", ErrorAction::Card},
    {0x0022, "ERR_SLAVE_INDEX_OUTRANG", "slave index out of range", ErrorAction::Ethercat},
    {0x0023, "ERR_AX_INDEX_OUTRANG", "axis index out of range", ErrorAction::Parameter},
    {0x0024, "ERR_AXCHN_INDEX_OUTRANG", "axis channel index out of range", ErrorAction::Ethercat},
    {0x0040, "ERR_PARAM_OUTRANG", "parameter out of range", ErrorAction::Parameter},
    {0x0041, "ERR_COUNT_OUTRANG", "count out of range", ErrorAction::Parameter},
    {0x0042, "ERR_LENGTH_OUTRANG", "length out of range", ErrorAction::Parameter},
    {0x0043, "ERR_METHOD_OUTRANG", "method out of range", ErrorAction::Parameter},
    {0x0044, "ERR_TYPE_OUTRANG", "type out of range", ErrorAction::Parameter},
    {0x0045, "ERR_OFFSET_OUTRANG", "offset out of range", ErrorAction::Parameter},
    {0x0046, "ERR_MODE_OUTRANG", "mode out of range", ErrorAction::Parameter},
    {0x0047, "ERR_BIT_OUTRANG", "bit index out of range", ErrorAction::Parameter},
    {0x0048, "ERR_CONDITION_OUTRANG", "condition out of range", ErrorAction::Parameter},
    {0x0060, "ERR_ONOFF_PARA", "on/off parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0061, "ERR_FLAG_PARA", "flag parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0062, "ERR_LEVEL_PARA", "level parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0063, "ERR_TRIG_PARA", "trigger parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0064, "ERR_DIR_PARA", "direction parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0065, "ERR_INVERSE_PARA", "inverse parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0066, "ERR_ENABLE_PARA", "enable parameter must be 0 or 1", ErrorAction::Parameter},
    {0x0080, "ERR_TIME_PARA_OUTRANG", "time parameter out of range", ErrorAction::Parameter},
    {0x0081, "ERR_FLTTIME_OUTRANG", "filter time out of range", ErrorAction::Parameter},
    {0x0100, "ERR_VEL_OUTRANG", "velocity out of range", ErrorAction::Motion},
    {0x0101, "ERR_ACC_OUTRANG", "acceleration out of range", ErrorAction::Motion},
    {0x0102, "ERR_DEC_OUTRANG", "deceleration out of range", ErrorAction::Motion},
    {0x0103, "ERR_TGTPOS_OUTRANG", "target position out of range", ErrorAction::Motion},
    {0x0104, "ERR_ENDVEL_OUTRANG (alias ERR_STVEL_OUTRANG)", "end/start velocity out of range", ErrorAction::EndVelocity},
    {0x0105, "ERR_RATIO_OUTRANG", "motion ratio out of range", ErrorAction::Motion},
    {0x0106, "ERR_EQV_OUTRANG", "equivalent unit out of range", ErrorAction::Motion},
    {0x0107, "ERR_TOL_OUTRANG", "position tolerance out of range", ErrorAction::Motion},
    {0x0109, "ERR_JERK_OUTRANG", "jerk out of range", ErrorAction::Motion},
    {0x0120, "ERR_BGVEL_OUTRANG", "begin velocity out of range", ErrorAction::Motion},
    {0x0121, "ERR_MAXVEL_OUTRANG", "maximum velocity out of range", ErrorAction::Motion},
    {0x0122, "ERR_MAXACC_OUTRANG", "maximum acceleration out of range", ErrorAction::Motion},
    {0x0123, "ERR_MAXDEC_OUTRANG", "maximum deceleration out of range", ErrorAction::Motion},
    {0x0124, "ERR_MAXJERK_OUTRANG", "maximum jerk out of range", ErrorAction::Motion},
    {0x0125, "ERR_STOPDEC_OUTRANG", "smooth-stop deceleration out of range", ErrorAction::Motion},
    {0x0126, "ERR_ESTOPDEC_OUTRANG", "emergency-stop deceleration out of range", ErrorAction::Motion},
    {0x0127, "ERR_ARRIVEDBAND_OUTRANG", "arrival band out of range", ErrorAction::Motion},
    {0x0128, "ERR_ERRLMT_OUTRANG", "following-error limit out of range", ErrorAction::Motion},
    {0x0160, "ERR_SLAVE_CNT_OUTRANG", "EtherCAT slave count out of range", ErrorAction::Ethercat},
    {0x0161, "ERR_ECAT_AX_CNT_OUTRANG", "EtherCAT axis count out of range", ErrorAction::Ethercat},
    {0x0162, "ERR_DIO_MOD_CNT_OUTRANG", "EtherCAT DIO module count out of range", ErrorAction::Ethercat},
    {0x0163, "ERR_AIO_MOD_CNT_OUTRANG", "EtherCAT AIO module count out of range", ErrorAction::Ethercat},
    {0x0164, "ERR_REG_MOD_CNT_OUTRANG", "EtherCAT register module count out of range", ErrorAction::Ethercat},
    {0x0200, "ERR_ECAT_MASTER_LINK", "EtherCAT master link error", ErrorAction::Ethercat},
    {0x0201, "ERR_ECAT_SLAVE_LINK", "EtherCAT slave link error", ErrorAction::Ethercat},
    {0x0210, "ERR_ECAT_PDO_LEN_OUTRANG", "EtherCAT PDO length out of range", ErrorAction::Ethercat},
    {0x0211, "ERR_ECAT_PDO_OFS_LEN", "EtherCAT PDO offset or length invalid", ErrorAction::Ethercat},
    {0x0212, "ERR_ECAT_PDO_OFS_BIT_LEN", "EtherCAT PDO bit offset crosses a byte", ErrorAction::Ethercat},
    {0x0218, "ERR_ECAT_PDO_NOT_EXIST", "required EtherCAT PDO does not exist", ErrorAction::Ethercat},
    {0x0219, "ERR_ECAT_PDO_NOT_SUPPORT", "required EtherCAT PDO is unsupported", ErrorAction::Ethercat},
    {0x0220, "ERR_PDO_CTRLWORD_NO_CFG", "control-word PDO 0x6040 is not configured", ErrorAction::Ethercat},
    {0x0221, "ERR_PDO_STSWORD_NO_CFG", "status-word PDO 0x6041 is not configured", ErrorAction::Ethercat},
    {0x0222, "ERR_PDO_TGTPOS_NO_CFG", "target-position PDO 0x607a is not configured", ErrorAction::Ethercat},
    {0x0223, "ERR_PDO_ATLPOS_NO_CFG", "actual-position PDO 0x6064 is not configured", ErrorAction::Ethercat},
    {0x0224, "ERR_PDO_TGTVEL_NO_CFG", "target-velocity PDO 0x60ff is not configured", ErrorAction::Ethercat},
    {0x0225, "ERR_PDO_ATLVEL_NO_CFG", "actual-velocity PDO 0x606c is not configured", ErrorAction::Ethercat},
    {0x0226, "ERR_PDO_TORQ_SLP_NO_CFG", "torque-slope PDO 0x6087 is not configured", ErrorAction::Ethercat},
    {0x0227, "ERR_PDO_TGT_TORQ_NO_CFG", "target-torque PDO 0x6071 is not configured", ErrorAction::Ethercat},
    {0x0228, "ERR_PDO_ATLTRQ_NO_CFG", "actual-torque PDO 0x6077 is not configured", ErrorAction::Ethercat},
    {0x0229, "ERR_PDO_ATLFERR_NO_CFG", "following-error PDO 0x60f4 is not configured", ErrorAction::Ethercat},
    {0x022a, "ERR_PDO_ERRCODE_NO_CFG", "error-code PDO 0x603f is not configured", ErrorAction::Ethercat},
    {0x022b, "ERR_PDO_AX_DI_CFG", "axis DI PDO 0x60fd is not configured", ErrorAction::Ethercat},
    {0x022c, "ERR_PDO_AX_DO_CFG", "axis DO PDO 0x60fe is not configured", ErrorAction::Ethercat},
    {0x022d, "ERR_PDO_OP_MODE_NO_CFG", "operation-mode PDO 0x6060 is not configured", ErrorAction::Ethercat},
    {0x0250, "ERR_CTRL_MODE", "invalid EtherCAT control mode", ErrorAction::Ethercat},
    {0x0251, "ERR_MODE_NOT_CSP", "axis is not in CSP mode", ErrorAction::Ethercat},
    {0x0252, "ERR_MODE_NOT_CSV", "axis is not in CSV mode", ErrorAction::Ethercat},
    {0x0253, "ERR_MODE_NOT_CST", "axis is not in CST mode", ErrorAction::Ethercat},
    {0x0254, "ERR_MODE_NOT_HOME", "axis is not in Home mode", ErrorAction::Ethercat},
    {0x0300, "ERR_ALRM_ENABLE", "alarm-enable parameter invalid", ErrorAction::Safety},
    {0x0301, "ERR_SOFTLMT_ENABLE", "soft-limit enable parameter invalid", ErrorAction::Safety},
    {0x0302, "ERR_HWLMT_ENABLE", "hardware-limit enable parameter invalid", ErrorAction::Safety},
    {0x0303, "ERR_ERRLMT_ENABLE", "following-error enable parameter invalid", ErrorAction::Safety},
    {0x0310, "ERR_HW_ESTP_IS_TRIG", "hardware emergency stop is triggered", ErrorAction::EmergencyStop},
    {0x0311, "ERR_AX_MV_DIR_LIMT_TRIG", "limit in the requested motion direction is triggered", ErrorAction::Safety},
    {0x0312, "ERR_AX_FOLLOW_ERROR", "axis following-error alarm", ErrorAction::Safety},
    {0x0318, "ERR_TGTPOS_OVER_SOFTLMT", "target position exceeds a software limit", ErrorAction::Safety},
    {0x0319, "ERR_SOFTLMT_POS_LESS_NEG", "positive software limit is below negative limit", ErrorAction::Safety},
    {0x0330, "ERR_PRF_VEL_NOT_ZERO", "planned velocity is not zero or axis has not arrived", ErrorAction::AxisState},
    {0x0331, "ERR_ENC_VEL_NOT_ZERO", "encoder velocity is not zero", ErrorAction::AxisState},
    {0x0338, "ERR_AX_SVON", "axis is already Servo On", ErrorAction::AxisState},
    {0x0339, "ERR_AX_ALARM", "servo alarm is active", ErrorAction::Safety},
    {0x033a, "ERR_AX_SVOFF", "axis is Servo Off", ErrorAction::ServoOff},
    {0x033c, "ERR_AX_BUSY", "axis is currently planning motion", ErrorAction::Busy},
    {0x033d, "ERR_AX_ABNOR", "axis abnormal alarm is active", ErrorAction::Safety},
    {0x0350, "ERR_AX_BOND_SAME_CHN", "axes are bound to the same physical channel", ErrorAction::Parameter},
    {0x0351, "ERR_AX_NOT_ECAT", "axis is not an EtherCAT servo axis", ErrorAction::Ethercat},
    {0x0360, "ERR_AX_MAPPED_CRD", "axis is mapped to an interpolation coordinate", ErrorAction::AxisState},
    {0x0361, "ERR_AX_MAPPED_MULTI", "axis is mapped to multi-axis mode", ErrorAction::AxisState},
    {0x0362, "ERR_AX_MAPPED_PT", "axis is mapped to PT mode", ErrorAction::AxisState},
    {0x0380, "ERR_PRF_MODE", "invalid motion planning mode", ErrorAction::AxisState},
    {0x0381, "ERR_PRF_MODE_NOT_PTP", "axis is not in PTP planning mode", ErrorAction::AxisState},
    {0x0382, "ERR_PRF_MODE_NOT_JOG", "axis is not in JOG planning mode", ErrorAction::AxisState},
    {0x0390, "ERR_HOMING_MODE", "axis is already homing", ErrorAction::Busy},
    {0x0391, "ERR_HOMING_CSP", "axis is in CSP homing", ErrorAction::Busy},
    {0x0392, "ERR_HOMING_CIA", "axis is in CiA402 homing", ErrorAction::Busy},
    {0x0393, "ERR_HOMING_NOT_CSP", "axis is not in CSP homing", ErrorAction::AxisState},
    {0x0394, "ERR_HOMING_NOT_CIA", "axis is not in CiA402 homing", ErrorAction::AxisState},
    {0x0400, "ERR_PTP_STOPPED", "PTP motion is already stopped", ErrorAction::AxisState},
    {0x0702, "ERR_NO_SYS_INT_SIGNAL", "system interrupt is not running; verify EtherCAT is operational", ErrorAction::Ethercat},
    {0x8000, "ERR_CARD_OPEN", "card has not been opened", ErrorAction::Card},
    {0x8001, "ERR_CARD_INDEX_READ", "failed to read card index", ErrorAction::Card},
    {0x8002, "ERR_CARD_INDEX_REPEAT", "duplicate card index detected", ErrorAction::Card},
    {0x8003, "ERR_CARD_CNT_OUTRANG", "scanned card count out of range", ErrorAction::Card},
    {0x8004, "ERR_CARD_NOT_FOUND", "configured card was not found", ErrorAction::Card},
    {0x8005, "ERR_CARD_OPEN_OPTION", "card open option is unsupported", ErrorAction::Card},
    {0x8006, "ERR_PCI_OPENDEV", "failed to open PCI device", ErrorAction::Card},
    {0x8020, "ERR_PCI2DSP_BUSY", "DSP remained busy", ErrorAction::Transport},
    {0x8021, "ERR_PCI2DSP_WAIT", "PCI-to-DSP response timed out", ErrorAction::Transport},
    {0x8022, "ERR_PCI2DSP_CHN", "PCI-to-DSP channel is abnormal", ErrorAction::Transport},
    {0x8023, "ERR_PCI2DSP_NORESP", "PCI-to-DSP command has no response", ErrorAction::Transport},
    {0x8200, "ERR_ECAT_MASTER_STS", "EtherCAT master bus error", ErrorAction::Ethercat},
    {0x8201, "ERR_ECAT_MASTER_NOT_OP_STS", "EtherCAT master is not in OP state", ErrorAction::Ethercat},
    {0x8202, "ERR_ECAT_NOT_ALIAS_MODE", "EtherCAT master is not in alias mode", ErrorAction::Ethercat},
    {0x8220, "ERR_ECAT_NOT_FIND_AXCHN", "EtherCAT axis channel was not found", ErrorAction::Ethercat},
    {0x8221, "ERR_ECAT_NOT_FIND_SLAVE", "EtherCAT slave was not found", ErrorAction::Ethercat},
    {0x8240, "ERR_ECAT_MASTER_OP_WAIT", "timed out waiting for EtherCAT OP state", ErrorAction::Ethercat},
    {0x8241, "ERR_ECAT_ENTER_HOME_WAIT", "timed out entering EtherCAT homing mode", ErrorAction::Ethercat},
    {0x8242, "ERR_ECAT_EXIT_HOME_WAIT", "timed out leaving EtherCAT homing mode", ErrorAction::Ethercat},
    {0x8280, "ERR_ECAT_SDO_DOWNLOAD_6060", "failed to download SDO 0x6060", ErrorAction::Ethercat},
    {0x8281, "ERR_ECAT_SDO_DOWNLOAD_6098", "failed to download SDO 0x6098", ErrorAction::Ethercat},
    {0x8282, "ERR_ECAT_SDO_DOWNLOAD_6099H", "failed to download SDO 0x6099 high speed", ErrorAction::Ethercat},
    {0x8283, "ERR_ECAT_SDO_DOWNLOAD_6099L", "failed to download SDO 0x6099 low speed", ErrorAction::Ethercat},
    {0x8284, "ERR_ECAT_SDO_DOWNLOAD_609A", "failed to download SDO 0x609a", ErrorAction::Ethercat},
    {0x8285, "ERR_ECAT_SDO_DOWNLOAD_607C", "failed to download SDO 0x607c", ErrorAction::Ethercat},
    {0x8290, "ERR_ECAT_SDO_UPLOAD_6060", "failed to upload SDO 0x6060", ErrorAction::Ethercat}
};

QString actionText(ErrorAction action)
{
    switch (action) {
    case ErrorAction::Transport:
        return QStringLiteral("\u64CD\u4F5C / Action: \u68C0\u67E5\u9A71\u52A8\u3001DLL \u4E0E\u901A\u4FE1\u94FE\u8DEF\u540E\u91CD\u8BD5 / check the driver, DLL, and communication link, then retry");
    case ErrorAction::Parameter:
        return QStringLiteral("\u64CD\u4F5C / Action: \u6821\u9A8C\u914D\u7F6E\u503C\u3001\u6307\u9488\u4EE5\u53CA card/axis \u6620\u5C04 / validate configuration values, pointers, and card/axis mapping");
    case ErrorAction::Card:
        return QStringLiteral("\u64CD\u4F5C / Action: \u786E\u8BA4\u53610\u5B58\u5728\u4E14\u672A\u88AB\u5176\u4ED6\u8FDB\u7A0B\u5360\u7528\uFF0C\u5FC5\u8981\u65F6\u91CD\u542F\u9A71\u52A8 / confirm card 0 exists and is not owned by another process; restart the driver if needed");
    case ErrorAction::Ethercat:
        return QStringLiteral("\u64CD\u4F5C / Action: \u68C0\u67E5\u4ECE\u7AD9\u4F9B\u7535\u3001\u7F51\u7EBF\u548C PDO/SDO \u914D\u7F6E\uFF0C\u7136\u540E\u91CD\u65B0\u626B\u63CF\u521D\u59CB\u5316 / check slave power, cabling, and PDO/SDO configuration, then rescan and initialize");
    case ErrorAction::Motion:
        return QStringLiteral("\u64CD\u4F5C / Action: \u505C\u8F74\u5E76\u6821\u9A8C\u901F\u5EA6\u3001\u52A0\u51CF\u901F\u4E0E\u76EE\u6807\u4F4D\u7F6E / stop the axis and validate velocity, acceleration/deceleration, and target position");
    case ErrorAction::Safety:
        return QStringLiteral("\u64CD\u4F5C / Action: \u505C\u6B62\u8FD0\u52A8\uFF0C\u68C0\u67E5\u9650\u4F4D\u65B9\u5411\u5E76\u6E05\u9664\u4F3A\u670D\u62A5\u8B66 / stop motion, verify the limit direction, and clear the servo alarm");
    case ErrorAction::AxisState:
        return QStringLiteral("\u64CD\u4F5C / Action: \u505C\u6B62\u8FD0\u52A8\u5E76\u7B49\u5F85\u8F74\u5230\u4F4D\uFF0C\u518D\u6062\u590D PTP/JOG \u6A21\u5F0F / stop motion and wait for arrival, then restore the required PTP/JOG mode");
    case ErrorAction::EndVelocity:
        return QStringLiteral("\u64CD\u4F5C / Action: \u5C06 stopSpeed \u8C03\u6574\u5230\u63A7\u5236\u5361\u5141\u8BB8\u7684\u7ED3\u675F\u901F\u5EA6\u8303\u56F4 / adjust stopSpeed to the controller's permitted end-velocity range");
    case ErrorAction::EmergencyStop:
        return QStringLiteral("\u64CD\u4F5C / Action: \u786E\u8BA4\u4EBA\u5458\u4E0E\u8BBE\u5907\u5B89\u5168\u540E\u89E3\u9664\u786C\u4EF6\u6025\u505C\uFF0C\u518D\u6E05\u72B6\u6001\u91CD\u8BD5 / after confirming safety, release the hardware emergency stop, clear status, and retry");
    case ErrorAction::ServoOff:
        return QStringLiteral("\u64CD\u4F5C / Action: \u6E05\u9664\u4F3A\u670D\u62A5\u8B66\u5E76\u6267\u884C Servo On\uFF0C\u786E\u8BA4\u4F7F\u80FD\u540E\u91CD\u8BD5 / clear servo alarms, issue Servo On, verify enablement, and retry");
    case ErrorAction::Busy:
        return QStringLiteral("\u64CD\u4F5C / Action: \u505C\u6B62\u5F53\u524D\u8FD0\u52A8\u5E76\u7B49\u5F85\u8F74\u7A7A\u95F2\u540E\u91CD\u8BD5 / stop the current motion and wait until the axis is idle before retrying");
    }
    return QString();
}

QString errorCodeText(int code)
{
    const auto describe = [](const ErrorInfo& info) {
        return QString("%1: %2. %3")
            .arg(QString::fromLatin1(info.symbol))
            .arg(QString::fromLatin1(info.explanation))
            .arg(actionText(info.action));
    };
    for (const ErrorInfo& info : kTaskErrorInfo) {
        if (info.code == code) {
            return describe(info);
        }
    }
    const int lowWord = static_cast<int>(static_cast<unsigned int>(code) & 0xffffU);
    if (lowWord != code) {
        for (const ErrorInfo& info : kTaskErrorInfo) {
            if (info.code == lowWord) {
                return describe(info)
                    + QString(" (decoded vendor low16=0x%1)").arg(lowWord, 4, 16, QChar('0'));
            }
        }
    }
    return QStringLiteral("UNRECOGNIZED_IMC_ERROR: \u672A\u8BC6\u522B\u7684 IMC errorcode.h \u9519\u8BEF / unrecognized IMC errorcode.h value. \u64CD\u4F5C / Action: \u4FDD\u7559\u6570\u503C\u5E76\u67E5\u8BE2\u5F53\u524D SDK errorcode.h\uFF0C\u505C\u6B62\u786C\u4EF6\u64CD\u4F5C / preserve the value, inspect the installed SDK errorcode.h, and stop hardware operation");
}

QString formatApiFailure(
    const char* functionName, int card, short axis, int code)
{
    const QString axisText = axis >= 0 ? QString::number(axis) : "n/a";
    return QString("%1 failed (card=%2, physical-axis=%3, code=%4 [0x%5], %6)")
        .arg(functionName)
        .arg(card)
        .arg(axisText)
        .arg(code)
        .arg(static_cast<unsigned int>(code), 0, 16)
        .arg(errorCodeText(code));
}

bool reachedLimit(unsigned int status, unsigned int reason, int direction)
{
    if (direction < 0) {
        return (status & kNegativeLimit) != 0 || reason == kNegativeLimitStopReason;
    }
    return (status & kPositiveLimit) != 0 || reason == kPositiveLimitStopReason;
}

qlonglong absolutePulseDifference(int lhs, int rhs)
{
    return std::abs(static_cast<qlonglong>(lhs) - static_cast<qlonglong>(rhs));
}

bool sdkIsPoisoned(QString* reason)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (reason) {
        *reason = gSdkShutdownPoisonReason;
    }
    return gSdkShutdownPoisoned;
}

} // namespace

Imc60gMotionController::Imc60gMotionController(
    IImc60gApi* api, const PrintHardwareProfile& profile, IImc60gClock* clock)
    : api_(api)
    , profile_(profile)
    , clock_(clock)
{
    if (!clock_) {
        ownedClock_ = std::make_unique<SystemImc60gClock>();
        clock_ = ownedClock_.get();
    }
}

Imc60gMotionController::~Imc60gMotionController()
{
    disconnect(nullptr);
}

Imc60gConnectionState Imc60gMotionController::state() const
{
    QMutexLocker locker(&mutex_);
    return state_;
}

bool Imc60gMotionController::acquireOwnership(QString* errorMessage)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (gSdkShutdownPoisoned) {
        setError(errorMessage,
            "IMC60G SDK access is blocked after an unverified shutdown; restart the process. "
            + gSdkShutdownPoisonReason);
        return false;
    }
    if (gSdkOwner && gSdkOwner != this) {
        setError(errorMessage,
            "IMC60G SDK card 0 is already owned by another motion controller.");
        return false;
    }
    gSdkOwner = this;
    return true;
}

void Imc60gMotionController::releaseOwnership()
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    if (gSdkOwner == this) {
        gSdkOwner = nullptr;
    }
}

void Imc60gMotionController::poisonOwnership(const QString& reason)
{
    QMutexLocker ownerLock(&gSdkOwnerMutex);
    gSdkShutdownPoisoned = true;
    gSdkShutdownPoisonReason = reason;
    if (gSdkOwner == this) {
        gSdkOwner = nullptr;
    }
}

bool Imc60gMotionController::callSucceeded(
    int code, const char* functionName, short axis, QString* errorMessage) const
{
    if (code == 0) {
        return true;
    }
    setError(errorMessage,
        formatApiFailure(functionName, profile_.cardIndex, axis, code));
    return false;
}

bool Imc60gMotionController::connectAndHome(QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }
    cancelRequested_.store(false);

    QString validationError;
    if (!validatePrintHardwareProfile(profile_, &validationError)) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage, validationError);
        return false;
    }
    if (state_ == Imc60gConnectionState::Ready) {
        return true;
    }
    if (!api_) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage, "IMC60G API is unavailable; no hardware call was made.");
        return false;
    }
    if (!acquireOwnership(errorMessage)) {
        state_ = Imc60gConnectionState::Fault;
        return false;
    }

    state_ = Imc60gConnectionState::Connecting;
    unsigned int cards = 0;
    unsigned int masterStatus = 0;
    Imc60gMasterInfo masterInfo;
    const int requiredAxisCount = std::max(profile_.axisX, profile_.axisY) + 1;
    if (!callSucceeded(api_->getCardsNum(&cards), "IMC_GetCardsNum", kNoAxis, errorMessage)) {
        goto fail;
    }
    if (cards == 0) {
        setError(errorMessage, "IMC_GetCardsNum found no IMC60G cards (card=0, count=0).");
        goto fail;
    }
    if (!callSucceeded(api_->openCard(0), "IMC_OpenCard", kNoAxis, errorMessage)) {
        goto fail;
    }
    cardOpened_ = true;
    ethercatTouched_ = true;
    if (!callSucceeded(api_->scanEthercat(0, 40), "IMC_ScanCardEcat", kNoAxis, errorMessage)) {
        goto fail;
    }

    if (!callSucceeded(api_->ethercatMasterStatus(0, &masterStatus),
            "IMC_GetEcatMasterSts", kNoAxis, errorMessage)
        || !callSucceeded(api_->ethercatMasterInfo(0, &masterInfo),
            "IMC_GetEcatMasterInfo", kNoAxis, errorMessage)) {
        goto fail;
    }
    if (masterStatus != kEthercatMasterOperational) {
        setError(errorMessage,
            QString("IMC60G EtherCAT master is not OP after automatic scan: status=%1 expected=6 (OP).")
                .arg(masterStatus));
        goto fail;
    }
    if (masterInfo.axisCount < requiredAxisCount) {
        setError(errorMessage,
            QString("IMC60G EtherCAT discovery found %1 axis resources; X/Y require physical axes 0 and 1.")
                .arg(masterInfo.axisCount));
        goto fail;
    }

    if (!callSucceeded(api_->setEmergencyLevel(0, 1), "IMC_SetEmgTrigLevelInv", kNoAxis, errorMessage)) {
        goto fail;
    }

    short emergencyStatus = 0;
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (!callSucceeded(api_->emergencyStatus(0, &emergencyStatus),
                "IMC_GetEmgSts", kNoAxis, errorMessage)) {
            goto fail;
        }
        if ((emergencyStatus & kHardwareEmergencyStop) == 0) {
            break;
        }
        if (attempt < 19) {
            clock_->sleepMs(50);
        }
    }
    if ((emergencyStatus & kHardwareEmergencyStop) != 0) {
        setError(errorMessage,
            QString("IMC60G emergency state remains active after automatic software release: emergency=0x%1.")
                .arg(static_cast<unsigned short>(emergencyStatus), 4, 16, QLatin1Char('0')));
        goto fail;
    }

    if (!callSucceeded(api_->clearAxisStatus(0, 0), "IMC_ClrAxSts", 0, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, 1), "IMC_ClrAxSts", 1, errorMessage)) {
        goto fail;
    }
    // Once Servo On is issued, shutdown must treat the axis as potentially
    // enabled even when the return code is nonzero.
    servoEnabled_[0] = true;
    if (!callSucceeded(api_->servoOn(0, 0), "IMC_ServoOn", 0, errorMessage)
        || !confirmServoOn(0, errorMessage)) {
        goto fail;
    }
    servoEnabled_[1] = true;
    if (!callSucceeded(api_->servoOn(0, 1), "IMC_ServoOn", 1, errorMessage)
        || !confirmServoOn(1, errorMessage)) {
        goto fail;
    }

    state_ = Imc60gConnectionState::Homing;
    if (!homeAxis(PrintHardwareProfile::LogicalAxis::Y, errorMessage)
        || !homeAxis(PrintHardwareProfile::LogicalAxis::X, errorMessage)) {
        goto fail;
    }
    state_ = Imc60gConnectionState::Ready;
    return true;

fail:
    {
        const QString primaryError = errorMessage ? *errorMessage : QString();
        QString cleanupError;
        const bool cleanupVerified = cleanupHardware(&cleanupError);
        if (!cleanupVerified) {
            poisonOwnership(cleanupError);
            setError(errorMessage,
                primaryError + "; shutdown unverified: " + cleanupError);
        } else {
            releaseOwnership();
        }
        state_ = Imc60gConnectionState::Fault;
        return false;
    }
}

bool Imc60gMotionController::cancellationRequested(
    short activeAxis, QString* errorMessage)
{
    if (!cancelRequested_.load()) {
        return false;
    }
    const int stopCode = api_->stop(0, activeAxis, 1);
    if (stopCode != 0) {
        setError(errorMessage,
            "IMC60G homing cancelled; "
            + formatApiFailure("IMC_StopMove", profile_.cardIndex, activeAxis, stopCode));
    } else {
        setError(errorMessage,
            QString("IMC60G homing cancelled (card=%1, physical-axis=%2).")
                .arg(profile_.cardIndex).arg(activeAxis));
    }
    return true;
}

bool Imc60gMotionController::confirmServoOn(short axis, QString* errorMessage)
{
    unsigned int status = 0;
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (!callSucceeded(api_->axisStatus(0, axis, &status),
                "IMC_GetAxSts", axis, errorMessage)) {
            return false;
        }
        if ((status & kAxisServoOn) != 0) {
            return true;
        }
        if ((status & (kAxisAlarm | kAxisEmergency | kAxisUnlinked)) != 0) {
            break;
        }
        if (attempt < 19) {
            clock_->sleepMs(50);
        }
    }
    setError(errorMessage,
        QString("IMC60G axis did not enter Servo On state after IMC_ServoOn: physical-axis=%1 status=0x%2.")
            .arg(axis)
            .arg(status, 8, 16, QLatin1Char('0')));
    return false;
}

bool Imc60gMotionController::homeAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage)
{
    const short axis = physicalAxis(logicalAxis);
    if (axis < 0) {
        setError(errorMessage, "IMC60G homing rejected: configured physical axis is missing.");
        return false;
    }
    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    const int direction = homeDirection(logicalAxis);
    if (direction != -1 && direction != 1) {
        setError(errorMessage,
            QString("IMC60G homing rejected invalid direction (card=0, physical-axis=%1).")
                .arg(axis));
        return false;
    }

    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis), "IMC_ClrAxSts", axis, errorMessage)) {
        return false;
    }

    unsigned int status = 0;
    unsigned int reason = 0;
    int startPlanned = 0;
    int startEncoder = 0;
    if (!callSucceeded(api_->axisStatus(0, axis, &status), "IMC_GetAxSts", axis, errorMessage)
        || !callSucceeded(api_->stopReason(0, axis, &reason), "IMC_GetAxStopReason", axis, errorMessage)
        || !callSucceeded(api_->plannedPosition(0, axis, &startPlanned), "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->encoderPosition(0, axis, &startEncoder), "IMC_GetAxEncPos32", axis, errorMessage)) {
        return false;
    }

    bool limitReached = reachedLimit(status, reason, direction);
    if (!limitReached) {
        if (!callSucceeded(api_->setMotionProfile(0, axis, profile_.homeSpeed,
                profile_.homeAcceleration, profile_.homeDeceleration, 0),
                "IMC_SetAxMvPara", axis, errorMessage)
            || !callSucceeded(api_->configureJog(0, axis),
                "IMC_JogPrf", axis, errorMessage)
            || !callSucceeded(api_->startJogMove(0, axis, direction),
                "IMC_StartJogMove", axis, errorMessage)) {
            return false;
        }

        const qint64 timeoutStart = clock_->nowMs();
        qint64 encoderStableSince = timeoutStart;
        int lastEncoder = startEncoder;
        while (clock_->nowMs() - timeoutStart <= profile_.homeTimeoutMs) {
            if (cancellationRequested(axis, errorMessage)) {
                return false;
            }
            int planned = 0;
            int encoder = 0;
            if (!callSucceeded(api_->axisStatus(0, axis, &status), "IMC_GetAxSts", axis, errorMessage)
                || !callSucceeded(api_->stopReason(0, axis, &reason), "IMC_GetAxStopReason", axis, errorMessage)
                || !callSucceeded(api_->plannedPosition(0, axis, &planned), "IMC_GetAxPrfPos32", axis, errorMessage)
                || !callSucceeded(api_->encoderPosition(0, axis, &encoder), "IMC_GetAxEncPos32", axis, errorMessage)) {
                return false;
            }
            if (absolutePulseDifference(encoder, lastEncoder) > 3) {
                lastEncoder = encoder;
                encoderStableSince = clock_->nowMs();
            }
            if (reachedLimit(status, reason, direction) || reason == kDiStopReason) {
                limitReached = true;
                break;
            }
            const bool movedEnough =
                absolutePulseDifference(encoder, startEncoder) >= profile_.homeMinimumMove;
            const bool stoppedAndStable = (status & kAxisBusy) == 0
                && clock_->nowMs() - encoderStableSince >= profile_.homeStableMs;
            if (movedEnough && stoppedAndStable) {
                limitReached = true;
                break;
            }
            if ((status & kAxisAlarm) != 0) {
                setError(errorMessage,
                    QString("IMC60G homing alarm (card=0, physical-axis=%1, status=0x%2, stop-reason=%3).")
                        .arg(axis).arg(status, 0, 16).arg(reason));
                return false;
            }
            clock_->sleepMs(1);
        }
        if (!limitReached) {
            setError(errorMessage,
                QString("IMC60G homing timed out (card=0, physical-axis=%1, timeout-ms=%2, status=0x%3, stop-reason=%4).")
                    .arg(axis).arg(profile_.homeTimeoutMs).arg(status, 0, 16).arg(reason));
            return false;
        }
    }

    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)) {
        return false;
    }

    int currentPosition = 0;
    if (!callSucceeded(api_->plannedPosition(0, axis, &currentPosition),
            "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis),
            "IMC_ClrAxSts", axis, errorMessage)
        || !callSucceeded(api_->setMotionProfile(0, axis, profile_.homeBackoffSpeed,
            profile_.homeAcceleration, profile_.homeDeceleration, 0),
            "IMC_SetAxMvPara", axis, errorMessage)) {
        return false;
    }
    const int backoffTarget = currentPosition + (-direction) * homeBackoff(logicalAxis);
    if (!callSucceeded(api_->startPtp(0, axis, backoffTarget),
            "IMC_StartPtpMove", axis, errorMessage)) {
        return false;
    }

    const qint64 backoffStart = clock_->nowMs();
    bool backoffComplete = false;
    while (clock_->nowMs() - backoffStart <= profile_.homeBackoffTimeoutMs) {
        if (cancellationRequested(axis, errorMessage)) {
            return false;
        }
        if (!callSucceeded(api_->axisStatus(0, axis, &status),
                "IMC_GetAxSts", axis, errorMessage)) {
            return false;
        }
        if ((status & kAxisBusy) == 0) {
            backoffComplete = true;
            break;
        }
        clock_->sleepMs(1);
    }
    if (!backoffComplete) {
        setError(errorMessage,
            QString("IMC60G backoff timed out (card=0, physical-axis=%1, timeout-ms=%2).")
                .arg(axis).arg(profile_.homeBackoffTimeoutMs));
        return false;
    }
    if (cancellationRequested(axis, errorMessage)) {
        return false;
    }
    if (!callSucceeded(api_->stop(0, axis, 0), "IMC_StopMove", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis), "IMC_ClrAxSts", axis, errorMessage)
        || !callSucceeded(api_->setCurrentPosition(0, axis, 0.0),
            "IMC_SetAxCurPos", axis, errorMessage)) {
        return false;
    }
    clock_->sleepMs(20);
    if (!callSucceeded(api_->syncPosition(0, axis), "IMC_SyncAxPos", axis, errorMessage)
        || !callSucceeded(api_->setCurrentPosition(0, axis, 0.0),
            "IMC_SetAxCurPos", axis, errorMessage)
        || !callSucceeded(api_->clearAxisStatus(0, axis),
            "IMC_ClrAxSts", axis, errorMessage)) {
        return false;
    }
    return true;
}

bool Imc60gMotionController::cleanupHardware(QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!api_ || !cardOpened_) {
        return true;
    }

    QStringList failures;
    const auto recordFailure = [&](int code, const char* operation, short axis) {
        if (code != 0) {
            failures << formatApiFailure(operation, profile_.cardIndex, axis, code);
        }
    };

    recordFailure(api_->stop(0, 0, 1), "IMC_StopMove", 0);
    recordFailure(api_->stop(0, 1, 1), "IMC_StopMove", 1);
    if (servoEnabled_[0]) {
        recordFailure(api_->servoOff(0, 0), "IMC_ServoOff", 0);
    }
    if (servoEnabled_[1]) {
        recordFailure(api_->servoOff(0, 1), "IMC_ServoOff", 1);
    }
    if (ethercatTouched_) {
        recordFailure(api_->stopEthercat(0), "IMC_DelEcatComm", kNoAxis);
    }
    recordFailure(api_->closeCard(0), "IMC_CloseCard", kNoAxis);

    servoEnabled_[0] = false;
    servoEnabled_[1] = false;
    ethercatTouched_ = false;
    cardOpened_ = false;

    if (!failures.isEmpty()) {
        setError(errorMessage, failures.join("; "));
        return false;
    }
    return true;
}

void Imc60gMotionController::requestCancellation()
{
    cancelRequested_.store(true);
}

bool Imc60gMotionController::disconnect(QString* errorMessage)
{
    requestCancellation();
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }

    QString poisonReason;
    if (sdkIsPoisoned(&poisonReason)) {
        state_ = Imc60gConnectionState::Fault;
        setError(errorMessage,
            "IMC60G remains in Fault after an unverified shutdown; restart the process. "
            + poisonReason);
        releaseOwnership();
        return false;
    }

    QString cleanupError;
    if (!cleanupHardware(&cleanupError)) {
        poisonOwnership(cleanupError);
        state_ = Imc60gConnectionState::Fault;
        printActive_ = false;
        setError(errorMessage,
            "IMC60G shutdown is unverified; restart the process. " + cleanupError);
        return false;
    }

    releaseOwnership();
    printActive_ = false;
    state_ = Imc60gConnectionState::Disconnected;
    return true;
}

short Imc60gMotionController::physicalAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    if (logicalAxis == PrintHardwareProfile::LogicalAxis::X) {
        return static_cast<short>(profile_.axisX);
    }
    if (logicalAxis == PrintHardwareProfile::LogicalAxis::Y) {
        return static_cast<short>(profile_.axisY);
    }
    return kNoAxis;
}

int Imc60gMotionController::homeDirection(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    return logicalAxis == PrintHardwareProfile::LogicalAxis::X
        ? profile_.homeDirectionX : profile_.homeDirectionY;
}

int Imc60gMotionController::homeBackoff(
    PrintHardwareProfile::LogicalAxis logicalAxis) const
{
    return logicalAxis == PrintHardwareProfile::LogicalAxis::X
        ? profile_.homeBackoffX : profile_.homeBackoffY;
}

bool Imc60gMotionController::moveRelative(
    PrintHardwareProfile::LogicalAxis logicalAxis, double millimeters,
    const PrintAxisConfig& axisConfig, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) {
        errorMessage->clear();
    }
    const short axis = physicalAxis(logicalAxis);
    if (state_ != Imc60gConnectionState::Ready) {
        setError(errorMessage, "Manual IMC60G motion requires an explicit successful connectAndHome().");
        return false;
    }
    if (printActive_) {
        setError(errorMessage, "Manual IMC60G motion is blocked while a print is active.");
        return false;
    }
    if (axis < 0) {
        setError(errorMessage, "Manual IMC60G motion rejected: logical axis has no real physical axis.");
        return false;
    }
    if (!std::isfinite(millimeters) || millimeters == 0.0
        || axisConfig.maxDistance <= 0.0
        || std::abs(millimeters) > axisConfig.maxDistance) {
        setError(errorMessage, "Manual IMC60G motion exceeds the configured maximum travel.");
        return false;
    }
    if (axisConfig.subdivision <= 0 || axisConfig.resolution <= 0
        || axisConfig.speedOfMovement <= 0 || axisConfig.acceleratedVelocity <= 0
        || axisConfig.startSpeed < 0 || axisConfig.stopSpeed < 0) {
        setError(errorMessage, "Manual IMC60G motion has invalid speed or unit configuration.");
        return false;
    }

    const double signedDistance = axisConfig.changeDirection ? -millimeters : millimeters;
    const double pulseValue =
        signedDistance * axisConfig.subdivision * axisConfig.resolution;
    if (!std::isfinite(pulseValue)
        || pulseValue > std::numeric_limits<int>::max()
        || pulseValue < std::numeric_limits<int>::min()) {
        setError(errorMessage, "Manual IMC60G target overflows the native 32-bit position.");
        return false;
    }
    int currentPosition = 0;
    if (!callSucceeded(api_->plannedPosition(0, axis, &currentPosition),
            "IMC_GetAxPrfPos32", axis, errorMessage)) {
        return false;
    }
    const qlonglong relativePulses = std::llround(pulseValue);
    const qlonglong target = static_cast<qlonglong>(currentPosition) + relativePulses;
    if (target > std::numeric_limits<int>::max()
        || target < std::numeric_limits<int>::min()) {
        setError(errorMessage, "Manual IMC60G target overflows the native 32-bit position.");
        return false;
    }
    if (!callSucceeded(api_->setMotionProfile(0, axis,
            axisConfig.speedOfMovement,
            axisConfig.acceleratedVelocity,
            axisConfig.acceleratedVelocity,
            axisConfig.startSpeed),
            "IMC_SetAxMvPara", axis, errorMessage)) {
        return false;
    }
    if (axisConfig.stopSpeed > 0
        && !callSucceeded(api_->setAxisEndVelocity(0, axis, axisConfig.stopSpeed),
            "IMC_SetAxEndVel", axis, errorMessage)) {
        return false;
    }
    return callSucceeded(api_->startPtp(0, axis, static_cast<int>(target)),
        "IMC_StartPtpMove", axis, errorMessage);
}

bool Imc60gMotionController::stopAxis(
    PrintHardwareProfile::LogicalAxis logicalAxis, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    const short axis = physicalAxis(logicalAxis);
    if (state_ != Imc60gConnectionState::Ready || axis < 0) {
        setError(errorMessage, "Cannot stop an unavailable IMC60G axis before connectAndHome().");
        return false;
    }
    return callSucceeded(api_->stop(0, axis, 1), "IMC_StopMove", axis, errorMessage);
}

bool Imc60gMotionController::readSnapshot(
    PrintHardwareProfile::LogicalAxis logicalAxis, Imc60gAxisSnapshot* snapshot,
    QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    const short axis = physicalAxis(logicalAxis);
    if (!snapshot) {
        setError(errorMessage, "IMC60G snapshot destination is null.");
        return false;
    }
    if (state_ != Imc60gConnectionState::Ready || axis < 0) {
        setError(errorMessage, "Cannot read an unavailable IMC60G axis before connectAndHome().");
        return false;
    }
    Imc60gAxisSnapshot value;
    value.logicalAxis = logicalAxis;
    value.physicalAxis = axis;
    if (!callSucceeded(api_->axisStatus(0, axis, &value.status), "IMC_GetAxSts", axis, errorMessage)
        || !callSucceeded(api_->stopReason(0, axis, &value.stopReason), "IMC_GetAxStopReason", axis, errorMessage)
        || !callSucceeded(api_->plannedPosition(0, axis, &value.plannedPosition), "IMC_GetAxPrfPos32", axis, errorMessage)
        || !callSucceeded(api_->encoderPosition(0, axis, &value.encoderPosition), "IMC_GetAxEncPos32", axis, errorMessage)) {
        return false;
    }
    *snapshot = value;
    return true;
}

bool Imc60gMotionController::isReadyForPrint() const
{
    QMutexLocker locker(&mutex_);
    return state_ == Imc60gConnectionState::Ready && !printActive_;
}

void Imc60gMotionController::setPrintActive(bool active)
{
    QMutexLocker locker(&mutex_);
    printActive_ = active;
}

PrintMotionReadiness Imc60gMotionController::printReadiness(
    QString* errorMessage) const
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) errorMessage->clear();
    PrintMotionReadiness readiness;
    readiness.sdkRuntimeReady = api_ != nullptr;
    readiness.axisMappingLocked = profile_.cardIndex == 0
        && profile_.axisX == 1 && profile_.axisY == 0;
    readiness.cardReady = state_ == Imc60gConnectionState::Ready;
    readiness.axesHomed = readiness.cardReady;
    if (!readiness.cardReady || !api_) {
        setError(errorMessage, "IMC60G requires an explicit successful connectAndHome().");
        return readiness;
    }

    QString detail;
    unsigned int masterStatus = 0;
    if (!callSucceeded(api_->ethercatMasterStatus(profile_.cardIndex,
            &masterStatus), "IMC_GetEcatMasterSts", kNoAxis, &detail)) {
        if (errorMessage) *errorMessage = detail;
        return readiness;
    }
    readiness.ethercatReady = masterStatus == kEthercatMasterOperational;

    short emergencyStatus = 0;
    if (!callSucceeded(api_->emergencyStatus(profile_.cardIndex,
            &emergencyStatus), "IMC_GetEmgSts", kNoAxis, &detail)) {
        if (errorMessage) *errorMessage = detail;
        return readiness;
    }
    readiness.emergencyClear = emergencyStatus == 0;

    unsigned int xStatus = 0;
    unsigned int yStatus = 0;
    const bool xOk = callSucceeded(api_->axisStatus(profile_.cardIndex,
        static_cast<short>(profile_.axisX), &xStatus), "IMC_GetAxSts",
        static_cast<short>(profile_.axisX), &detail);
    const bool yOk = callSucceeded(api_->axisStatus(profile_.cardIndex,
        static_cast<short>(profile_.axisY), &yStatus), "IMC_GetAxSts",
        static_cast<short>(profile_.axisY), &detail);
    const unsigned int unsafeMask =
        kAxisAlarm | kAxisEmergency | kAxisUnlinked;
    readiness.servosReady = xOk && yOk
        && (xStatus & kAxisServoOn) != 0
        && (yStatus & kAxisServoOn) != 0
        && (xStatus & unsafeMask) == 0
        && (yStatus & unsafeMask) == 0;
    readiness.axesStopped = xOk && yOk
        && (xStatus & (unsafeMask | kAxisBusy)) == 0
        && (yStatus & (unsafeMask | kAxisBusy)) == 0;
    readiness.emergencyClear = readiness.emergencyClear
        && xOk && yOk
        && (xStatus & kAxisEmergency) == 0
        && (yStatus & kAxisEmergency) == 0;
    readiness.ethercatReady = readiness.ethercatReady
        && xOk && yOk
        && (xStatus & kAxisUnlinked) == 0
        && (yStatus & kAxisUnlinked) == 0;
    if ((!xOk || !yOk) && errorMessage) *errorMessage = detail;
    if (errorMessage && errorMessage->isEmpty()) {
        if (!readiness.ethercatReady) {
            *errorMessage = QString("EtherCAT master/axis link is not operational: master=%1 x=0x%2 y=0x%3.")
                .arg(masterStatus)
                .arg(xStatus, 8, 16, QLatin1Char('0'))
                .arg(yStatus, 8, 16, QLatin1Char('0'));
        } else if (!readiness.emergencyClear) {
            *errorMessage = QString("IMC60G emergency state is active: emergency=0x%1 x=0x%2 y=0x%3.")
                .arg(static_cast<unsigned short>(emergencyStatus), 4, 16, QLatin1Char('0'))
                .arg(xStatus, 8, 16, QLatin1Char('0'))
                .arg(yStatus, 8, 16, QLatin1Char('0'));
        } else if (!readiness.servosReady) {
            *errorMessage = QString("X/Y Servo On verification failed: x=0x%1 y=0x%2.")
                .arg(xStatus, 8, 16, QLatin1Char('0'))
                .arg(yStatus, 8, 16, QLatin1Char('0'));
        } else if (!readiness.axesStopped) {
            *errorMessage = QString("X/Y stopped verification failed: x=0x%1 y=0x%2.")
                .arg(xStatus, 8, 16, QLatin1Char('0'))
                .arg(yStatus, 8, 16, QLatin1Char('0'));
        }
    }
    return readiness;
}

bool Imc60gMotionController::beginPrint(QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) errorMessage->clear();
    if (state_ != Imc60gConnectionState::Ready || printActive_) {
        setError(errorMessage,
            "IMC60G print ownership requires Ready state and no active print.");
        return false;
    }
    printActive_ = true;
    return true;
}

void Imc60gMotionController::endPrint()
{
    QMutexLocker locker(&mutex_);
    printActive_ = false;
}

bool Imc60gMotionController::startPrintMove(short axis, qint32 absoluteTarget,
    const PrintAxisConfig& config, QString* errorMessage)
{
    if (state_ != Imc60gConnectionState::Ready || !printActive_) {
        setError(errorMessage,
            "IMC60G print move requires already-connected print ownership.");
        return false;
    }
    if (axis < 0 || config.speedOfMovement <= 0
        || config.acceleratedVelocity <= 0 || config.startSpeed < 0
        || config.stopSpeed < 0) {
        setError(errorMessage, "IMC60G print motion profile is invalid.");
        return false;
    }
    if (!callSucceeded(api_->setMotionProfile(profile_.cardIndex, axis,
            config.speedOfMovement, config.acceleratedVelocity,
            config.acceleratedVelocity, config.startSpeed),
            "IMC_SetAxMvPara", axis, errorMessage)) {
        return false;
    }
    if (!callSucceeded(api_->setAxisEndVelocity(profile_.cardIndex, axis,
            config.stopSpeed), "IMC_SetAxEndVel", axis, errorMessage)) {
        return false;
    }
    return callSucceeded(api_->startPtp(profile_.cardIndex, axis, absoluteTarget),
        "IMC_StartPtpMove", axis, errorMessage);
}

bool Imc60gMotionController::startYScan(qint32 absoluteTarget,
    const PrintAxisConfig& config, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) errorMessage->clear();
    return startPrintMove(static_cast<short>(profile_.axisY), absoluteTarget,
        config, errorMessage);
}

bool Imc60gMotionController::waitPrintAxisStopped(short axis, int timeoutMs,
    const std::atomic_bool* cancelRequested, QString* errorMessage)
{
    if (timeoutMs <= 0) {
        setError(errorMessage, "IMC60G wait timeout must be positive.");
        return false;
    }
    const qint64 started = clock_->nowMs();
    while (true) {
        if (cancelRequested && cancelRequested->load()) {
            setError(errorMessage, "IMC60G print wait was cancelled.");
            return false;
        }
        unsigned int status = 0;
        if (!callSucceeded(api_->axisStatus(profile_.cardIndex, axis, &status),
                "IMC_GetAxSts", axis, errorMessage)) {
            return false;
        }
        if ((status & kAxisAlarm) != 0) {
            setError(errorMessage,
                QString("IMC60G axis alarm while waiting: axis=%1 status=0x%2")
                    .arg(axis).arg(status, 8, 16, QLatin1Char('0')));
            return false;
        }
        if ((status & kAxisBusy) == 0) {
            unsigned int reason = 0;
            if (!callSucceeded(api_->stopReason(profile_.cardIndex, axis, &reason),
                    "IMC_GetAxStopReason", axis, errorMessage)) {
                return false;
            }
            if (reason == kPositiveLimitStopReason
                || reason == kNegativeLimitStopReason
                || reason == kDiStopReason) {
                setError(errorMessage,
                    QString("IMC60G print axis stopped by safety input: axis=%1 reason=0x%2")
                        .arg(axis).arg(reason, 8, 16, QLatin1Char('0')));
                return false;
            }
            return true;
        }
        if (clock_->nowMs() - started >= timeoutMs) {
            setError(errorMessage,
                QString("IMC60G axis stop wait timed out: axis=%1 timeoutMs=%2")
                    .arg(axis).arg(timeoutMs));
            return false;
        }
        clock_->sleepMs(2);
    }
}

bool Imc60gMotionController::waitYStopped(int timeoutMs,
    const std::atomic_bool& cancelRequested, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    return waitPrintAxisStopped(static_cast<short>(profile_.axisY), timeoutMs,
        &cancelRequested, errorMessage);
}

bool Imc60gMotionController::stepX(qint32 relativePulses,
    const PrintAxisConfig& config, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    if (errorMessage) errorMessage->clear();
    if (state_ != Imc60gConnectionState::Ready || !printActive_) {
        setError(errorMessage,
            "IMC60G X row step requires already-connected print ownership.");
        return false;
    }
    const short axis = static_cast<short>(profile_.axisX);
    int current = 0;
    if (!callSucceeded(api_->plannedPosition(profile_.cardIndex, axis, &current),
            "IMC_GetAxPrfPos32", axis, errorMessage)) {
        return false;
    }
    const qint64 target = static_cast<qint64>(current) + relativePulses;
    if (target < std::numeric_limits<qint32>::min()
        || target > std::numeric_limits<qint32>::max()) {
        setError(errorMessage, "IMC60G X row-step target exceeds int32.");
        return false;
    }
    return startPrintMove(axis, static_cast<qint32>(target), config, errorMessage);
}

bool Imc60gMotionController::waitXStopped(int timeoutMs,
    const std::atomic_bool& cancelRequested, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    return waitPrintAxisStopped(static_cast<short>(profile_.axisX), timeoutMs,
        &cancelRequested, errorMessage);
}

bool Imc60gMotionController::stopMappedAxes(QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    QString errors;
    QString detail;
    const short y = static_cast<short>(profile_.axisY);
    const short x = static_cast<short>(profile_.axisX);
    if (!callSucceeded(api_->stop(profile_.cardIndex, y, 1),
            "IMC_StopMove", y, &detail)) {
        appendTaskError(&errors, "Y stop failed: " + detail);
    }
    detail.clear();
    if (!callSucceeded(api_->stop(profile_.cardIndex, x, 1),
            "IMC_StopMove", x, &detail)) {
        appendTaskError(&errors, "X stop failed: " + detail);
    }
    setError(errorMessage, errors);
    return errors.isEmpty();
}

bool Imc60gMotionController::waitMappedAxesStopped(int timeoutMs,
    QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    QString errors;
    QString detail;
    if (!waitPrintAxisStopped(static_cast<short>(profile_.axisY), timeoutMs,
            nullptr, &detail)) {
        appendTaskError(&errors, "Y stopped verification failed: " + detail);
    }
    detail.clear();
    if (!waitPrintAxisStopped(static_cast<short>(profile_.axisX), timeoutMs,
            nullptr, &detail)) {
        appendTaskError(&errors, "X stopped verification failed: " + detail);
    }
    setError(errorMessage, errors);
    return errors.isEmpty();
}

bool Imc60gMotionController::returnToLogicalZero(
    const PrintAxisConfig& xConfig, const PrintAxisConfig& yConfig,
    int timeoutMs, QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    QString errors;
    QString detail;
    const short x = static_cast<short>(profile_.axisX);
    const short y = static_cast<short>(profile_.axisY);
    if (!startPrintMove(x, 0, xConfig, &detail)) {
        appendTaskError(&errors, "X zero move failed: " + detail);
    } else if (!waitPrintAxisStopped(x, timeoutMs, nullptr, &detail)) {
        appendTaskError(&errors, "X zero wait failed: " + detail);
    }
    detail.clear();
    if (!startPrintMove(y, 0, yConfig, &detail)) {
        appendTaskError(&errors, "Y zero move failed: " + detail);
    } else if (!waitPrintAxisStopped(y, timeoutMs, nullptr, &detail)) {
        appendTaskError(&errors, "Y zero wait failed: " + detail);
    }
    setError(errorMessage, errors);
    return errors.isEmpty();
}

bool Imc60gMotionController::verifyLogicalZero(QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    QString errors;
    for (short axis : {static_cast<short>(profile_.axisX),
             static_cast<short>(profile_.axisY)}) {
        unsigned int status = 0;
        int planned = 0;
        int encoder = 0;
        QString detail;
        if (!callSucceeded(api_->axisStatus(profile_.cardIndex, axis, &status),
                "IMC_GetAxSts", axis, &detail)
            || !callSucceeded(api_->plannedPosition(profile_.cardIndex, axis, &planned),
                "IMC_GetAxPrfPos32", axis, &detail)
            || !callSucceeded(api_->encoderPosition(profile_.cardIndex, axis, &encoder),
                "IMC_GetAxEncPos32", axis, &detail)) {
            appendTaskError(&errors, detail);
            continue;
        }
        if ((status & (kAxisAlarm | kAxisBusy)) != 0
            || planned != 0 || std::abs(encoder) > profile_.homeMinimumMove) {
            appendTaskError(&errors,
                QString("Logical-zero verification failed: axis=%1 status=0x%2 planned=%3 encoder=%4 tolerance=%5")
                    .arg(axis).arg(status, 8, 16, QLatin1Char('0'))
                    .arg(planned).arg(encoder).arg(profile_.homeMinimumMove));
        }
    }
    setError(errorMessage, errors);
    return errors.isEmpty();
}
