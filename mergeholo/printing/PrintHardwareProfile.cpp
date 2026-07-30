#include "PrintHardwareProfile.h"

#include <QFileInfo>
#include <QSettings>

#include <limits>

namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

bool sameHomeOrder(const QVector<PrintHardwareProfile::LogicalAxis>& order)
{
    return order == QVector<PrintHardwareProfile::LogicalAxis>({
        PrintHardwareProfile::LogicalAxis::Y,
        PrintHardwareProfile::LogicalAxis::X
    });
}

bool requireValue(QSettings& settings, const QString& key, QVariant* value, QString* errorMessage)
{
    if (!settings.contains(key)) {
        setError(errorMessage, "IMC60G profile is missing required key: " + key);
        return false;
    }
    *value = settings.value(key);
    return true;
}

bool requireInt(QSettings& settings, const QString& key, int* value, QString* errorMessage)
{
    QVariant raw;
    if (!requireValue(settings, key, &raw, errorMessage)) {
        return false;
    }
    bool ok = false;
    const int parsed = raw.toString().toInt(&ok);
    if (!ok) {
        setError(errorMessage, "IMC60G profile key must be an integer: " + key);
        return false;
    }
    *value = parsed;
    return true;
}

bool requireLong(QSettings& settings, const QString& key, long* value, QString* errorMessage)
{
    QVariant raw;
    if (!requireValue(settings, key, &raw, errorMessage)) {
        return false;
    }
    bool ok = false;
    const qlonglong parsed = raw.toString().toLongLong(&ok);
    if (!ok) {
        setError(errorMessage, "IMC60G profile key must be an integer: " + key);
        return false;
    }
    if (parsed < static_cast<qlonglong>(std::numeric_limits<long>::min())
        || parsed > static_cast<qlonglong>(std::numeric_limits<long>::max())) {
        setError(errorMessage, "IMC60G profile key is outside the long integer range: " + key);
        return false;
    }
    *value = static_cast<long>(parsed);
    return true;
}

bool requireBool(QSettings& settings, const QString& key, bool* value, QString* errorMessage)
{
    QVariant raw;
    if (!requireValue(settings, key, &raw, errorMessage)) {
        return false;
    }
    const QString text = raw.toString().trimmed().toLower();
    if (text != "true" && text != "false" && text != "1" && text != "0") {
        setError(errorMessage, "IMC60G profile key must be a boolean: " + key);
        return false;
    }
    *value = text == "true" || text == "1";
    return true;
}

bool requireHomeOrder(QSettings& settings, QVector<PrintHardwareProfile::LogicalAxis>* value, QString* errorMessage)
{
    QVariant raw;
    if (!requireValue(settings, "home_order", &raw, errorMessage)) {
        return false;
    }
    const QString homeOrder = raw.type() == QVariant::StringList
        ? raw.toStringList().join(",")
        : raw.toString().trimmed();
    if (homeOrder != "Y,X") {
        setError(errorMessage, "IMC60G profile home_order must be Y,X.");
        return false;
    }
    *value = {PrintHardwareProfile::LogicalAxis::Y, PrintHardwareProfile::LogicalAxis::X};
    return true;
}

bool requirePrintDefaults(QSettings& settings, QString* errorMessage)
{
    int addTempPulse = 0;
    int leadPulse = 0;
    if (!requireInt(settings, "default_add_temp_pulse", &addTempPulse, errorMessage)
        || !requireInt(settings, "default_lead_pulse", &leadPulse, errorMessage)) {
        return false;
    }
    if (addTempPulse != 16000 || leadPulse != 1000) {
        setError(errorMessage, "IMC60G profile print defaults differ from the approved production profile.");
        return false;
    }
    return true;
}

bool checkSettingsStatus(const QSettings& settings, QString* errorMessage)
{
    if (settings.status() == QSettings::NoError) {
        return true;
    }
    if (settings.status() == QSettings::FormatError) {
        setError(errorMessage, "Malformed IMC60G profile INI.");
    } else {
        setError(errorMessage, "Cannot read IMC60G profile INI.");
    }
    return false;
}

} // namespace

PrintHardwareProfile loadPrintHardwareProfile(const QString& path, QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    PrintHardwareProfile profile;
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        setError(errorMessage, "IMC60G profile does not exist: " + path);
        profile.version = 0;
        return profile;
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");
    settings.allKeys();
    if (!checkSettingsStatus(settings, errorMessage)) {
        profile.version = 0;
        return profile;
    }
    settings.beginGroup("profile");
    if (!requireInt(settings, "version", &profile.version, errorMessage)
        || !requireInt(settings, "card_index", &profile.cardIndex, errorMessage)
        || !requireInt(settings, "axis_x", &profile.axisX, errorMessage)
        || !requireInt(settings, "axis_y", &profile.axisY, errorMessage)
        || !requireHomeOrder(settings, &profile.homeOrder, errorMessage)) {
        settings.endGroup();
        profile.version = 0;
        return profile;
    }
    settings.endGroup();

    settings.beginGroup("homing");
    if (!requireInt(settings, "direction_x", &profile.homeDirectionX, errorMessage)
        || !requireInt(settings, "direction_y", &profile.homeDirectionY, errorMessage)
        || !requireInt(settings, "speed", &profile.homeSpeed, errorMessage)
        || !requireInt(settings, "acceleration", &profile.homeAcceleration, errorMessage)
        || !requireInt(settings, "deceleration", &profile.homeDeceleration, errorMessage)
        || !requireInt(settings, "timeout_ms", &profile.homeTimeoutMs, errorMessage)
        || !requireInt(settings, "stable_ms", &profile.homeStableMs, errorMessage)
        || !requireInt(settings, "minimum_move_pulse", &profile.homeMinimumMove, errorMessage)
        || !requireInt(settings, "backoff_x_pulse", &profile.homeBackoffX, errorMessage)
        || !requireInt(settings, "backoff_y_pulse", &profile.homeBackoffY, errorMessage)
        || !requireInt(settings, "backoff_speed", &profile.homeBackoffSpeed, errorMessage)
        || !requireInt(settings, "backoff_timeout_ms", &profile.homeBackoffTimeoutMs, errorMessage)) {
        settings.endGroup();
        profile.version = 0;
        return profile;
    }
    settings.endGroup();

    settings.beginGroup("print");
    if (!requireLong(settings, "step_pulse", &profile.printStepPulse, errorMessage)
        || !requireLong(settings, "forward_delay_pulse", &profile.forwardDelayPulse, errorMessage)
        || !requireLong(settings, "reverse_fixed_pulse", &profile.reverseFixedPulse, errorMessage)
        || !requirePrintDefaults(settings, errorMessage)
        || !requireLong(settings, "exposure_offset_pulse", &profile.exposureOffsetPulse, errorMessage)) {
        settings.endGroup();
        profile.version = 0;
        return profile;
    }
    settings.endGroup();

    settings.beginGroup("sv660n");
    if (!requireInt(settings, "do_function", &profile.sv660nDoFunction, errorMessage)
        || !requireInt(settings, "point_index", &profile.sv660nPointIndex, errorMessage)
        || !requireInt(settings, "mode", &profile.sv660nMode, errorMessage)
        || !requireInt(settings, "width", &profile.sv660nWidth, errorMessage)
        || !requireBool(settings, "use_user_unit", &profile.sv660nUserUnits, errorMessage)
        || !requireInt(settings, "positive_attribute", &profile.sv660nPositiveAttribute, errorMessage)
        || !requireInt(settings, "negative_attribute", &profile.sv660nNegativeAttribute, errorMessage)) {
        settings.endGroup();
        profile.version = 0;
        return profile;
    }
    settings.endGroup();

    if (!checkSettingsStatus(settings, errorMessage)) {
        profile.version = 0;
        return profile;
    }
    if (!validatePrintHardwareProfile(profile, errorMessage)) {
        profile.version = 0;
        return profile;
    }
    return profile;
}

bool validatePrintHardwareProfile(const PrintHardwareProfile& profile, QString* errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    const PrintHardwareProfile approved;
    bool valid = true;
    if (profile.version != approved.version) {
        setError(errorMessage, "Unsupported IMC60G profile version.");
        valid = false;
    } else if (profile.cardIndex != approved.cardIndex) {
        setError(errorMessage, "IMC60G profile card index must be 0.");
        valid = false;
    } else if (profile.axisX == profile.axisY || profile.axisX != approved.axisX || profile.axisY != approved.axisY) {
        setError(errorMessage, "IMC60G profile must map logical X to axis 1 and Y to axis 0.");
        valid = false;
    } else if (!sameHomeOrder(profile.homeOrder)) {
        setError(errorMessage, "IMC60G profile home order must be logical Y then logical X.");
        valid = false;
    } else if (profile.homeDirectionX != approved.homeDirectionX || profile.homeDirectionY != approved.homeDirectionY
        || profile.homeSpeed != approved.homeSpeed || profile.homeAcceleration != approved.homeAcceleration
        || profile.homeDeceleration != approved.homeDeceleration || profile.homeTimeoutMs != approved.homeTimeoutMs
        || profile.homeStableMs != approved.homeStableMs || profile.homeMinimumMove != approved.homeMinimumMove
        || profile.homeBackoffX != approved.homeBackoffX || profile.homeBackoffY != approved.homeBackoffY
        || profile.homeBackoffSpeed != approved.homeBackoffSpeed || profile.homeBackoffTimeoutMs != approved.homeBackoffTimeoutMs
        || profile.printStepPulse != approved.printStepPulse || profile.forwardDelayPulse != approved.forwardDelayPulse
        || profile.reverseFixedPulse != approved.reverseFixedPulse || profile.exposureOffsetPulse != approved.exposureOffsetPulse
        || profile.sv660nDoFunction != approved.sv660nDoFunction || profile.sv660nPointIndex != approved.sv660nPointIndex
        || profile.sv660nMode != approved.sv660nMode || profile.sv660nWidth != approved.sv660nWidth
        || profile.sv660nUserUnits != approved.sv660nUserUnits
        || profile.sv660nPositiveAttribute != approved.sv660nPositiveAttribute
        || profile.sv660nNegativeAttribute != approved.sv660nNegativeAttribute) {
        setError(errorMessage, "IMC60G profile differs from the approved production profile.");
        valid = false;
    }
    return valid;
}
