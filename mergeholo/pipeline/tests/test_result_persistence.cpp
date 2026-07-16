#include "PipelineConfig.h"
#include "PipelineContext.h"
#include "ResultSaveSettings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(1);
    }
}

class TempDirectory {
public:
    TempDirectory()
    {
        path_ = fs::temp_directory_path() / "mergeholo_result_persistence_tests";
        fs::remove_all(path_);
        fs::create_directories(path_);
    }

    ~TempDirectory()
    {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    fs::path writeIni(const std::string& contents) const
    {
        const fs::path path = path_ / "test.ini";
        std::ofstream output(path, std::ios::out | std::ios::trunc);
        output << contents;
        output.close();
        return path;
    }

    const fs::path& path() const
    {
        return path_;
    }

private:
    fs::path path_;
};

void testDefaultsAreMemoryOnly()
{
    const TempDirectory temp;
    const fs::path ini = temp.writeIni(
        "output_root=output\n"
        "multiview_out_dir=multiview\n"
        "elemental_out_dir=elemental\n");

    HoloConfig config;
    applyConfig(config, ini);

    expect(!config.saveSettings.mesh, "mesh persistence must default off");
    expect(!config.saveSettings.multiview, "multiview persistence must default off");
    expect(!config.saveSettings.elemental, "elemental persistence must default off");
    expect(config.resultTimestamp.empty(), "timestamp must default empty");
    expect(config.meshOutDir == (temp.path() / "output" / "mesh").lexically_normal(),
        "mesh output default must resolve below output_root");
}

void testExplicitCombinationAndTimestamp()
{
    const TempDirectory temp;
    const fs::path ini = temp.writeIni(
        "output_root=output\n"
        "mesh_out_dir=saved_mesh\n"
        "save_mesh_result=true\n"
        "save_multiview_result=false\n"
        "save_elemental_result=true\n"
        "result_timestamp=20260716_153045_123\n");

    HoloConfig config;
    applyConfig(config, ini);

    expect(config.saveSettings.mesh, "mesh flag was not parsed");
    expect(!config.saveSettings.multiview, "multiview flag was not parsed");
    expect(config.saveSettings.elemental, "elemental flag was not parsed");
    expect(config.resultTimestamp == "20260716_153045_123", "timestamp was not parsed");
    expect(config.meshOutDir == (temp.path() / "output" / "saved_mesh").lexically_normal(),
        "mesh_out_dir must resolve below output_root");
}

void testSaveReportLifecycle()
{
    ResultSaveReport report;
    expect(!report.hasWarnings(), "new report must be empty");

    report.addWarning("mesh", fs::path("output/mesh_stamp"), "disk full");
    expect(report.hasWarnings(), "warning was not recorded");
    expect(report.warnings().size() == 1, "warning count mismatch");
    expect(report.warnings().front().resultType == "mesh", "warning result type mismatch");
    expect(report.warnings().front().outputDirectory == fs::path("output/mesh_stamp"),
        "warning output directory mismatch");
    expect(report.warnings().front().message == "disk full", "warning message mismatch");

    report.clear();
    expect(!report.hasWarnings(), "clear must remove warnings");
}

} // namespace

int main()
{
    testDefaultsAreMemoryOnly();
    testExplicitCombinationAndTimestamp();
    testSaveReportLifecycle();
    std::cout << "result persistence config tests passed\n";
    return 0;
}
