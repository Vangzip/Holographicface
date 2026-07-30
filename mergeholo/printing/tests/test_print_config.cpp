#include "PrintConfig.h"
#include "PrintHardwareProfile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

#ifndef IMC60G_PROFILE_PATH
#error IMC60G_PROFILE_PATH must identify the repository production profile
#endif

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << "\n";
        std::exit(1);
    }
}

QString productionProfilePath()
{
    return QString::fromUtf8(IMC60G_PROFILE_PATH);
}

QByteArray productionProfileContents()
{
    QFile file(productionProfilePath());
    expect(file.open(QIODevice::ReadOnly), "production profile should be readable");
    return file.readAll();
}

QString writeProfile(
    QTemporaryDir& dir,
    const QByteArray& contents,
    const QString& fileName = QStringLiteral("imc60g_print.ini"))
{
    const QString path = QDir(dir.path()).filePath(fileName);
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "temporary profile should be writable");
    expect(file.write(contents) == contents.size(), "temporary profile should be written completely");
    file.close();
    return path;
}

QString writeModifiedProfile(
    QTemporaryDir& dir,
    const QByteArray& original,
    const QByteArray& replacement)
{
    QByteArray contents = productionProfileContents();
    expect(contents.contains(original), "profile fixture should contain replacement source");
    contents.replace(original, replacement);
    return writeProfile(dir, contents);
}

void expectRejectedProfile(const QString& path, const char* message)
{
    QString error;
    const PrintHardwareProfile profile = loadPrintHardwareProfile(path, &error);
    expect(profile.version == 0, message);
    expect(!error.isEmpty(), message);
}

void testImc60gProductionProfileMatchesV2()
{
    QString error;
    const PrintHardwareProfile p = loadPrintHardwareProfile(productionProfilePath(), &error);
    expect(error.isEmpty(), "hardware profile should load");
    expect(p.version == 1 && p.cardIndex == 0, "card profile should be versioned");
    expect(p.axisX == 1 && p.axisY == 0, "X/Y mapping must match V2");
    expect(p.homeOrder == QVector<PrintHardwareProfile::LogicalAxis>({
        PrintHardwareProfile::LogicalAxis::Y,
        PrintHardwareProfile::LogicalAxis::X
    }), "home order must be logical Y then logical X");
    expect(p.homeDirectionX == -1 && p.homeDirectionY == -1, "home direction must match V2");
    expect(p.homeBackoffX == 28000 && p.homeBackoffY == 92000, "backoff pulses must match current V2 source");
    expect(p.printStepPulse == 1000 && p.forwardDelayPulse == 4000, "IMC print basis must match V2");
    expect(p.reverseFixedPulse == 2000 && p.exposureOffsetPulse == 2000, "IMC timing constants must match V2");
    expect(p.sv660nDoFunction == 25 && p.sv660nPointIndex == 1, "SV660N DO1 mapping must match V2");
    expect(p.sv660nPositiveAttribute == 129 && p.sv660nNegativeAttribute == 130,
        "SV660N crossing attributes must match V2");
}

void testPrintConfigDefaultsMatchV2()
{
    const Print9030Config config = defaultPrint9030Config();
    expect(config.configVersion == 2, "print config defaults must use version 2");
    expect(config.axisX.subdivision == 40 && config.axisX.resolution == 50, "X must use 2000 units/mm");
    expect(config.axisX.speedOfMovement == 5000, "X speed must match V2 aixs_config.cfg");
    expect(config.axisY.speedOfMovement == 60000, "Y speed must match V2 aixs_config.cfg");
    expect(config.main.moveAdjustMm == 20.0, "move adjustment must match V2 main_config.cfg");
    expect(config.main.widthScale == 3.8 && config.main.heightScale == 2.8, "scale must match V2");
    expect(config.main.addTempPulse == 16000 && config.main.leadPulse == 1000,
        "IMC pulse defaults must match V2");
    expect(!config.axisW.electricalStatus, "V2 defaults must not simulate exposure through W");
}

void testPrintConfigMigratesEditableValuesWithoutLegacyShutter()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    const QString path = QDir(dir.path()).filePath("legacy_print.ini");
    QFile file(path);
    expect(file.open(QIODevice::WriteOnly | QIODevice::Text), "legacy config should be writable");
    file.write("[main]\nmove_adjust_mm=17\ngrid_columns=12\n"
               "[axis_w]\nelectrical_status=true\n");
    file.close();

    QString error;
    const Print9030Config config = loadPrint9030Config(path, &error);
    expect(error.isEmpty(), "legacy config migration should not set an error");
    expect(config.configVersion == 2, "legacy config should migrate to version 2");
    expect(config.main.moveAdjustMm == 17.0 && config.main.gridColumns == 12,
        "legacy editable values should be retained");
    expect(!config.axisW.electricalStatus, "legacy W electrical status must not become a simulated shutter");

    expect(savePrint9030Config(path, config, &error), "migrated config should save");
    QSettings saved(path, QSettings::IniFormat);
    expect(saved.value("meta/version").toInt() == 2, "saved config should declare version 2");
    expect(!saved.value("axis_w/electrical_status").toBool(), "saved config must keep W shutter disabled");
}

void testUnknownProfileVersionIsRejected()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    expectRejectedProfile(
        writeModifiedProfile(dir, "version=1", "version=2"),
        "unknown profile version must be rejected");
}

void testMissingRequiredProfileKeyIsRejected()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    expectRejectedProfile(
        writeModifiedProfile(dir, "card_index=0", ""),
        "missing required profile key must be rejected");
}

void testAlteredLockedProfileValueIsRejected()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    expectRejectedProfile(
        writeModifiedProfile(dir, "backoff_x_pulse=28000", "backoff_x_pulse=28001"),
        "altered locked profile value must be rejected");
}

void expectLongOverflowRejected(const QByteArray& value)
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    QString error;
    const PrintHardwareProfile profile = loadPrintHardwareProfile(
        writeModifiedProfile(dir, "step_pulse=1000", "step_pulse=" + value), &error);
    expect(profile.version == 0, "out-of-range long must invalidate the profile");
    expect(error.contains("range", Qt::CaseInsensitive), "out-of-range long must report a range error");
}

void testProfileLongOverflowsAreRejectedBeforeNarrowing()
{
    expectLongOverflowRejected("4294968296"); // Approved 1000 modulo 2^32.
    expectLongOverflowRejected("2147483648");
    expectLongOverflowRejected("-2147483649");
}

void testMalformedIniIsRejectedAsMalformed()
{
    QTemporaryDir dir;
    expect(dir.isValid(), "temporary directory should be valid");
    QString error;
    const PrintHardwareProfile profile = loadPrintHardwareProfile(
        writeProfile(dir, "[profile\nversion=1\n"), &error);
    expect(profile.version == 0, "malformed INI must invalidate the profile");
    expect(error.contains("malformed", Qt::CaseInsensitive), "malformed INI must report its parse failure");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    testImc60gProductionProfileMatchesV2();
    testPrintConfigDefaultsMatchV2();
    testPrintConfigMigratesEditableValuesWithoutLegacyShutter();
    testUnknownProfileVersionIsRejected();
    testMissingRequiredProfileKeyIsRejected();
    testAlteredLockedProfileValueIsRejected();
    testMalformedIniIsRejectedAsMalformed();
    testProfileLongOverflowsAreRejectedBeforeNarrowing();
    std::cout << "print config tests passed\n";
    return 0;
}
