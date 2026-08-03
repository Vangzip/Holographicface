#include "PipelineConfig.h"
#include "PipelineContext.h"
#include "ExternalDepthOrientation.h"
#include "PipelineInput.h"
#include "ResultPersistence.h"
#include "ResultSaveSettings.h"
#include "ElementalProcessor.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <QCoreApplication>

#include <opencv2/imgcodecs.hpp>

#include <pcl/conversions.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#ifdef _WIN32
#include <process.h>
#endif

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
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
#ifdef _WIN32
        const int processId = _getpid();
#else
        const int processId = 0;
#endif
        path_ = fs::temp_directory_path()
            / ("mergeholo_result_persistence_tests_"
                + std::to_string(processId) + "_" + std::to_string(ticks));
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

void expectPlan(
    PipelineInputMode mode,
    bool depth,
    bool mesh,
    bool model,
    bool multiview,
    bool elemental,
    bool preloadMesh,
    bool useFileMultiview,
    bool readOnlySource)
{
    const PipelineRunPlan plan = makePipelineRunPlan(mode);
    expect(plan.depth == depth, "pipeline input depth plan mismatch");
    expect(plan.mesh == mesh, "pipeline input mesh plan mismatch");
    expect(plan.model == model, "pipeline input model plan mismatch");
    expect(plan.multiview == multiview, "pipeline input multiview plan mismatch");
    expect(plan.elemental == elemental, "pipeline input elemental plan mismatch");
    expect(plan.preloadMesh == preloadMesh,
        "pipeline input mesh preload plan mismatch");
    expect(plan.useFileMultiview == useFileMultiview,
        "pipeline input file multiview plan mismatch");
    expect(plan.readOnlySource == readOnlySource,
        "pipeline input read-only source plan mismatch");
}

void testExternalDepthPointCloudAxisNormalization()
{
    cv::Mat legacyDepth(4, 3, CV_32FC3);
    for (int row = 0; row < legacyDepth.rows; ++row) {
        for (int column = 0; column < legacyDepth.cols; ++column) {
            legacyDepth.at<cv::Vec3f>(row, column) = cv::Vec3f(
                -static_cast<float>(column), -static_cast<float>(row), 1.0f);
        }
    }

    pcl::PointCloud<pcl::PointXYZRGB> legacyCloud;
    legacyCloud.resize(2);
    legacyCloud[0].x = 1.0f;
    legacyCloud[0].y = -2.0f;
    legacyCloud[0].z = 3.0f;
    legacyCloud[1].x = -4.0f;
    legacyCloud[1].y = 4.0f;
    const ExternalDepthOrientationResult legacyResult =
        normalizeExternalDepthPointCloudAxes(legacyDepth, &legacyCloud);
    expect(legacyResult.horizontalDetermined,
        "legacy external depth horizontal orientation must be determined");
    expect(legacyResult.flippedPointCloudX,
        "legacy external depth must flip point-cloud X");
    expect(legacyResult.columnToPointCloudXCorrelation < -0.9,
        "legacy external depth must have leftward-positive point-cloud X");
    expect(legacyResult.verticalDetermined,
        "legacy external depth vertical orientation must be determined");
    expect(legacyResult.flippedPointCloudY,
        "legacy external depth must flip point-cloud Y");
    expect(legacyResult.rowToPointCloudYCorrelation > 0.9,
        "legacy external depth must have downward-positive point-cloud Y");
    expect(legacyCloud[0].x == -1.0f && legacyCloud[0].y == 2.0f
            && legacyCloud[0].z == 3.0f && legacyCloud[1].x == 4.0f
            && legacyCloud[1].y == -4.0f,
        "legacy external depth must flip point-cloud X and Y only");

    cv::Mat currentDepth(4, 3, CV_32FC3);
    for (int row = 0; row < currentDepth.rows; ++row) {
        for (int column = 0; column < currentDepth.cols; ++column) {
            currentDepth.at<cv::Vec3f>(row, column) = cv::Vec3f(
                static_cast<float>(column), static_cast<float>(row), 1.0f);
        }
    }

    pcl::PointCloud<pcl::PointXYZRGB> currentCloud;
    currentCloud.resize(1);
    currentCloud[0].x = 4.0f;
    currentCloud[0].y = 5.0f;
    const ExternalDepthOrientationResult currentResult =
        normalizeExternalDepthPointCloudAxes(currentDepth, &currentCloud);
    expect(currentResult.horizontalDetermined,
        "current external depth horizontal orientation must be determined");
    expect(!currentResult.flippedPointCloudX,
        "current external depth must keep point-cloud X");
    expect(currentResult.columnToPointCloudXCorrelation > 0.9,
        "current external depth must have rightward-positive point-cloud X");
    expect(currentResult.verticalDetermined,
        "current external depth vertical orientation must be determined");
    expect(!currentResult.flippedPointCloudY,
        "current external depth must keep point-cloud Y");
    expect(currentResult.rowToPointCloudYCorrelation < -0.9,
        "current external depth must have upward-positive point-cloud Y");
    expect(currentCloud[0].x == 4.0f && currentCloud[0].y == 5.0f,
        "current external depth must not mutate point-cloud X or Y");

    cv::Mat unknownDepth(2, 2, CV_8UC1, cv::Scalar(7));
    pcl::PointCloud<pcl::PointXYZRGB> unknownCloud;
    unknownCloud.resize(1);
    unknownCloud[0].x = 7.0f;
    unknownCloud[0].y = 6.0f;
    const ExternalDepthOrientationResult unknownResult =
        normalizeExternalDepthPointCloudAxes(unknownDepth, &unknownCloud);
    expect(!unknownResult.horizontalDetermined && !unknownResult.verticalDetermined
            && !unknownResult.flippedPointCloudX && !unknownResult.flippedPointCloudY,
        "unsupported external depth must remain undetermined");
    expect(unknownCloud[0].x == 7.0f && unknownCloud[0].y == 6.0f,
        "undetermined external depth must not mutate point-cloud Y");
}

void testPipelineInputModesAndRunPlans()
{
    expectPlan(PipelineInputMode::Camera,
        true, true, true, true, true, false, false, false);
    expectPlan(PipelineInputMode::RgbDepth,
        true, true, true, true, true, false, false, true);
    expectPlan(PipelineInputMode::Mesh,
        false, false, false, true, true, true, false, true);
    expectPlan(PipelineInputMode::Multiview,
        false, false, false, false, true, false, true, true);

    PipelineInputMode parsed = PipelineInputMode::Camera;
    expect(parsePipelineInputMode("rgb_depth", &parsed),
        "rgb_depth mode must parse");
    expect(parsed == PipelineInputMode::RgbDepth,
        "rgb_depth mode parsed to the wrong value");
    expect(pipelineInputModeName(PipelineInputMode::Multiview) == "multiview",
        "multiview mode must serialize");
    expect(!parsePipelineInputMode("invalid", &parsed),
        "invalid input mode must be rejected");
}

PipelineInputSelection inputSelection(PipelineInputMode mode, const fs::path& directory)
{
    PipelineInputSelection selection;
    selection.mode = mode;
    selection.directory = directory;
    return selection;
}

MultiviewInputSpec smallMultiviewSpec()
{
    MultiviewInputSpec spec;
    spec.viewRows = 2;
    spec.viewCols = 2;
    spec.viewNameDigits = 2;
    spec.viewWidth = 4;
    spec.viewHeight = 3;
    return spec;
}

void writeSolidImage(const fs::path& path, int width = 4, int height = 3)
{
    const cv::Mat image(height, width, CV_8UC3, cv::Scalar(20, 80, 220));
    expect(cv::imwrite(path.string(), image), "test image could not be written");
}

void writeDepthImage(const fs::path& path, int width = 4, int height = 3)
{
    const cv::Mat image(height, width, CV_32FC3, cv::Scalar(1.0f, 2.0f, 3.0f));
    const std::vector<int> params = { cv::IMWRITE_TIFF_COMPRESSION, 1 };
    expect(cv::imwrite(path.string(), image, params), "test depth TIFF could not be written");
}

void testPipelineInputConfigParsing()
{
    const TempDirectory temp;
    const fs::path ini = temp.writeIni(
        "input_mode=mesh\n"
        "input_dir=external_mesh\n");

    HoloConfig config;
    applyConfig(config, ini);
    expect(config.inputMode == PipelineInputMode::Mesh,
        "pipeline input mode was not parsed");
    expect(config.inputDirectory == (temp.path() / "external_mesh").lexically_normal(),
        "pipeline input directory was not resolved from the config directory");
}

void testRgbDepthInputValidation()
{
    const TempDirectory temp;
    const fs::path uppercaseDirectory = temp.path() / "uppercase";
    fs::create_directories(uppercaseDirectory);
    writeSolidImage(uppercaseDirectory / "face.jpg");
    writeDepthImage(uppercaseDirectory / "face.TIFF");

    PipelineInputFiles files;
    std::string error;
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::RgbDepth, uppercaseDirectory),
        smallMultiviewSpec(), &files, &error),
        "uppercase TIFF extension must be rejected to match the depth stage contract");

    writeSolidImage(temp.path() / "face.jpg");
    writeDepthImage(temp.path() / "face.tiff");

    expect(resolvePipelineInput(
        inputSelection(PipelineInputMode::RgbDepth, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "matching RGB/depth pair must resolve");
    expect(files.rgbPath.filename() == "face.jpg", "resolved RGB path is wrong");
    expect(files.depthPath.filename() == "face.tiff", "resolved depth path is wrong");

    writeSolidImage(temp.path() / "second.jpg");
    writeDepthImage(temp.path() / "second.tiff");
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::RgbDepth, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "multiple RGB/depth pairs must be rejected");

    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::RgbDepth, temp.path() / "missing"),
        smallMultiviewSpec(), &files, &error),
        "missing input directory must be rejected");
}

void testMeshInputValidation()
{
    const TempDirectory temp;
    const fs::path uppercaseDirectory = temp.path() / "uppercase";
    fs::create_directories(uppercaseDirectory);
    {
        std::ofstream mesh(uppercaseDirectory / "face_MESH.PLY");
        mesh << "ply\n";
    }
    writeSolidImage(uppercaseDirectory / "face.jpg");

    PipelineInputFiles files;
    std::string error;
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::Mesh, uppercaseDirectory),
        smallMultiviewSpec(), &files, &error),
        "uppercase mesh suffix must be rejected to match the mesh stage contract");

    const fs::path corruptTextureDirectory = temp.path() / "corrupt_texture";
    fs::create_directories(corruptTextureDirectory);
    {
        std::ofstream mesh(corruptTextureDirectory / "face_mesh.ply");
        mesh << "ply\n";
        std::ofstream texture(corruptTextureDirectory / "face.jpg");
        texture << "not a JPEG";
    }
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::Mesh, corruptTextureDirectory),
        smallMultiviewSpec(), &files, &error),
        "mesh input with an unreadable JPEG must be rejected");

    {
        std::ofstream mesh(temp.path() / "face_mesh.ply");
        mesh << "ply\n";
    }

    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::Mesh, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "mesh without its matching texture must be rejected");

    writeSolidImage(temp.path() / "face.jpg");
    expect(resolvePipelineInput(
        inputSelection(PipelineInputMode::Mesh, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "matching mesh/texture pair must resolve");
    expect(files.meshPath.filename() == "face_mesh.ply", "resolved mesh path is wrong");
    expect(files.rgbPath.filename() == "face.jpg", "resolved mesh texture path is wrong");
}

void testMultiviewInputValidation()
{
    const TempDirectory temp;
    const std::vector<std::string> names = {
        "0101.jpg", "0102.jpg", "0201.jpg", "0202.jpg"
    };
    for (const std::string& name : names) {
        writeSolidImage(temp.path() / name);
    }

    PipelineInputFiles files;
    std::string error;
    expect(resolvePipelineInput(
        inputSelection(PipelineInputMode::Multiview, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "complete multiview grid must resolve");
    expect(files.multiviewDirectory == temp.path(),
        "resolved multiview directory is wrong");

    fs::remove(temp.path() / "0102.jpg");
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::Multiview, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "missing middle multiview image must be rejected");

    writeSolidImage(temp.path() / "0102.jpg");
    writeSolidImage(temp.path() / "0101.jpg", 5, 3);
    expect(!resolvePipelineInput(
        inputSelection(PipelineInputMode::Multiview, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "wrong multiview image dimensions must be rejected");
}

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

void testCorrectedMultiviewConventionDoesNotFlipElementalRowsByDefault()
{
    const TempDirectory temp;
    const fs::path ini = temp.writeIni(
        "output_root=output\n"
        "multiview_out_dir=multiview\n"
        "elemental_out_dir=elemental\n");

    HoloConfig config;
    applyConfig(config, ini);

    expect(!config.elementalFlipSourceY,
        "corrected multiview source rows must stay upright by default");
    expect(!config.elementalFlipViewRows,
        "corrected multiview view rows must stay top-to-bottom by default");
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

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::in | std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

void replaceAll(std::string& text, const std::string& from, const std::string& to)
{
    size_t offset = 0;
    while ((offset = text.find(from, offset)) != std::string::npos) {
        text.replace(offset, from.size(), to);
        offset += to.size();
    }
}

void testUiTemplateSaveSettings()
{
    const fs::path executableDirectory(
        QCoreApplication::applicationDirPath().toStdWString());
    const fs::path templatePath = (executableDirectory
        / ".." / ".." / ".." / "config" / "ui_pipeline_template.ini").lexically_normal();
    std::string generatedConfig = readTextFile(templatePath);
    expect(!generatedConfig.empty(), "UI pipeline template could not be read");
    expect(generatedConfig.find("mesh_out_dir=mesh") != std::string::npos,
        "UI template must define the mesh output directory");
    expect(generatedConfig.find("save_mesh_result={{save_mesh_result}}") != std::string::npos,
        "UI template is missing the mesh save placeholder");
    expect(generatedConfig.find("save_multiview_result={{save_multiview_result}}") != std::string::npos,
        "UI template is missing the multiview save placeholder");
    expect(generatedConfig.find("save_elemental_result={{save_elemental_result}}") != std::string::npos,
        "UI template is missing the elemental save placeholder");
    expect(generatedConfig.find("result_timestamp={{result_timestamp}}") != std::string::npos,
        "UI template is missing the result timestamp placeholder");

    replaceAll(generatedConfig, "{{output_root}}", "output");
    replaceAll(generatedConfig, "{{depth_input_dir}}", "input");
    replaceAll(generatedConfig, "{{depth_config}}", "depth.cfg");
    replaceAll(generatedConfig, "{{mesh_config}}", "mesh.cfg");
    replaceAll(generatedConfig, "{{log_file}}", "pipeline.log");
    replaceAll(generatedConfig, "{{save_mesh_result}}", "true");
    replaceAll(generatedConfig, "{{save_multiview_result}}", "false");
    replaceAll(generatedConfig, "{{save_elemental_result}}", "true");
    replaceAll(generatedConfig, "{{result_timestamp}}", "20260716_153045_123");

    const TempDirectory temp;
    const fs::path ini = temp.writeIni(generatedConfig);
    HoloConfig config;
    applyConfig(config, ini);
    expect(config.saveSettings.mesh, "generated UI config lost the mesh selection");
    expect(!config.saveSettings.multiview,
        "generated UI config changed the multiview selection");
    expect(config.saveSettings.elemental,
        "generated UI config lost the elemental selection");
    expect(config.resultTimestamp == "20260716_153045_123",
        "generated UI config lost the run timestamp");
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

void testTimestampedDirectory()
{
    expect(
        timestampedResultDirectory(fs::path("output/mesh"), "20260716_153045_123")
            == fs::path("output/mesh_20260716_153045_123"),
        "timestamp suffix was not applied");
}

MeshMemoryResult makeTriangleMesh()
{
    pcl::PointCloud<pcl::PointXYZRGB> cloud;
    cloud.width = 3;
    cloud.height = 1;
    cloud.is_dense = true;
    cloud.points.resize(3);
    cloud.points[0].x = 0.0f;
    cloud.points[0].y = 0.0f;
    cloud.points[0].z = 0.0f;
    cloud.points[1].x = 1.0f;
    cloud.points[1].y = 0.0f;
    cloud.points[1].z = 0.0f;
    cloud.points[2].x = 0.0f;
    cloud.points[2].y = 1.0f;
    cloud.points[2].z = 0.0f;
    for (pcl::PointXYZRGB& point : cloud.points) {
        point.r = 220;
        point.g = 80;
        point.b = 20;
    }

    MeshMemoryResult result;
    result.mesh.reset(new pcl::PolygonMesh);
    pcl::toPCLPointCloud2(cloud, result.mesh->cloud);
    pcl::Vertices triangle;
    triangle.vertices = { 0, 1, 2 };
    result.mesh->polygons.push_back(triangle);
    return result;
}

MeshMemoryResult makeColorlessTriangleMesh()
{
    pcl::PointCloud<pcl::PointXYZ> cloud;
    cloud.width = 3;
    cloud.height = 1;
    cloud.is_dense = true;
    cloud.points.resize(3);
    cloud.points[0].x = 0.0f;
    cloud.points[0].y = 0.0f;
    cloud.points[0].z = 0.0f;
    cloud.points[1].x = 1.0f;
    cloud.points[1].y = 0.0f;
    cloud.points[1].z = 0.0f;
    cloud.points[2].x = 0.0f;
    cloud.points[2].y = 1.0f;
    cloud.points[2].z = 0.0f;

    MeshMemoryResult result;
    result.mesh.reset(new pcl::PolygonMesh);
    pcl::toPCLPointCloud2(cloud, result.mesh->cloud);
    pcl::Vertices triangle;
    triangle.vertices = { 0, 1, 2 };
    result.mesh->polygons.push_back(triangle);
    return result;
}

void testPipelineMeshInputLoading()
{
    const TempDirectory temp;
    const fs::path meshPath = temp.path() / "face_mesh.ply";
    MeshMemoryResult triangle = makeTriangleMesh();
    expect(pcl::io::savePLYFile(meshPath.string(), *triangle.mesh) == 0,
        "mesh input fixture could not be written");
    writeSolidImage(temp.path() / "face.jpg");

    PipelineInputFiles files;
    std::string error;
    expect(resolvePipelineInput(
        inputSelection(PipelineInputMode::Mesh, temp.path()),
        smallMultiviewSpec(), &files, &error),
        "mesh input fixture did not resolve");

    MeshMemoryResult loaded;
    expect(loadPipelineMeshInput(files, &loaded, &error),
        "mesh input must load");
    expect(loaded.hasMesh(), "loaded mesh must contain cloud and polygons");
    expect(loaded.baseName == "face", "mesh basename must remove _mesh suffix");
    expect(loaded.meshPath == files.meshPath,
        "logical mesh path must be preserved");
    expect(loaded.rgbPath == files.rgbPath,
        "texture path must be preserved");

    const fs::path colorlessPath = temp.path() / "colorless_mesh.ply";
    MeshMemoryResult colorless = makeColorlessTriangleMesh();
    expect(pcl::io::savePLYFile(colorlessPath.string(), *colorless.mesh) == 0,
        "colorless mesh input fixture could not be written");
    files.meshPath = colorlessPath;
    expect(!loadPipelineMeshInput(files, &loaded, &error),
        "mesh input without RGB/RGBA vertex colors must fail");
    expect(!loaded.hasMesh(),
        "failed colorless mesh load must clear the output result");

    const fs::path corruptPath = temp.path() / "broken_mesh.ply";
    {
        std::ofstream corrupt(corruptPath, std::ios::out | std::ios::trunc);
        corrupt << "not a ply file";
    }
    files.meshPath = corruptPath;
    expect(!loadPipelineMeshInput(files, &loaded, &error),
        "corrupt mesh input must fail");
    expect(!loaded.hasMesh() && loaded.meshPath.empty() && loaded.rgbPath.empty(),
        "failed mesh load must clear the output result");
}

void testMeshPersistence()
{
    const TempDirectory temp;
    HoloConfig config;
    config.meshOutDir = temp.path() / "output" / "mesh";
    config.resultTimestamp = "20260716_153045_123";
    MeshMemoryResult mesh = makeTriangleMesh();
    ResultSaveReport report;

    persistMeshResult(config, mesh, report);

    expect(!report.hasWarnings(), "mesh persistence unexpectedly warned");
    const fs::path outputPath = timestampedResultDirectory(
        config.meshOutDir, config.resultTimestamp) / "0_mesh.ply";
    expect(fs::exists(outputPath), "mesh PLY was not created");

    pcl::PolygonMesh reloaded;
    expect(pcl::io::loadPLYFile(outputPath.string(), reloaded) == 0,
        "mesh PLY could not be reloaded");
    expect(!reloaded.cloud.data.empty(), "reloaded mesh cloud is empty");
    expect(reloaded.polygons.size() == 1, "reloaded mesh polygon count mismatch");
}

void testMeshFileFallbackPersistence()
{
    const TempDirectory temp;
    const fs::path sourcePath = temp.path() / "source_mesh.ply";
    MeshMemoryResult sourceMesh = makeTriangleMesh();
    expect(pcl::io::savePLYFile(sourcePath.string(), *sourceMesh.mesh) == 0,
        "mesh file fallback fixture could not be written");

    MeshMemoryResult fileResult;
    fileResult.meshPath = sourcePath;
    HoloConfig config;
    config.meshOutDir = temp.path() / "output" / "mesh";
    config.resultTimestamp = "file_fallback";
    ResultSaveReport report;

    persistMeshResult(config, fileResult, report);

    expect(!report.hasWarnings(), "mesh file fallback unexpectedly warned");
    const fs::path outputPath = timestampedResultDirectory(
        config.meshOutDir, config.resultTimestamp) / "0_mesh.ply";
    pcl::PolygonMesh reloaded;
    expect(pcl::io::loadPLYFile(outputPath.string(), reloaded) == 0,
        "copied mesh fallback PLY could not be reloaded");
    expect(!reloaded.cloud.data.empty() && reloaded.polygons.size() == 1,
        "copied mesh fallback PLY content mismatch");
}

void fillRgbImage(
    unsigned char* pixels,
    int rows,
    int cols,
    bool bottomUp)
{
    for (int memoryRow = 0; memoryRow < rows; ++memoryRow) {
        const int logicalRow = bottomUp ? rows - 1 - memoryRow : memoryRow;
        const bool topHalf = logicalRow < rows / 2;
        for (int col = 0; col < cols; ++col) {
            unsigned char* pixel = pixels
                + (static_cast<size_t>(memoryRow) * static_cast<size_t>(cols)
                    + static_cast<size_t>(col)) * 3;
            pixel[0] = topHalf ? 255 : 0;
            pixel[1] = 0;
            pixel[2] = topHalf ? 0 : 255;
        }
    }
}

void expectTopRedBottomBlue(const cv::Mat& image, const char* label)
{
    expect(!image.empty(), label);
    const cv::Vec3b top = image.at<cv::Vec3b>(image.rows / 4, image.cols / 2);
    const cv::Vec3b bottom = image.at<cv::Vec3b>(image.rows * 3 / 4, image.cols / 2);
    expect(top[2] > 180 && top[0] < 80, "saved top pixel is not red");
    expect(bottom[0] > 180 && bottom[2] < 80, "saved bottom pixel is not blue");
}

void testMultiviewPersistence()
{
    const TempDirectory temp;
    HoloConfig config;
    config.multiviewOutDir = temp.path() / "output" / "multiview";
    config.resultTimestamp = "20260716_153045_123";
    config.viewNameDigits = 2;
    config.jpgQuality = 100;
    config.multiviewCamera.captureFlipVertical = true;

    MultiviewMemoryResult result;
    result.plan = std::make_shared<MultiviewRenderPlan>(2, 1, 16);
    result.sink = std::make_shared<MemoryFrameSink>(*result.plan, true);
    for (std::uint64_t frame = 0; frame < result.sink->frameCount(); ++frame) {
        fillRgbImage(result.sink->frameData(frame), 16, 16, true);
    }

    ResultSaveReport report;
    persistMultiviewResult(config, result, report);

    expect(!report.hasWarnings(), "multiview persistence unexpectedly warned");
    const fs::path outputDirectory = timestampedResultDirectory(
        config.multiviewOutDir, config.resultTimestamp);
    const std::vector<std::string> names = {
        "0101.jpg", "0102.jpg", "0201.jpg", "0202.jpg"
    };
    for (const std::string& name : names) {
        const fs::path outputPath = outputDirectory / name;
        expect(fs::exists(outputPath), "multiview JPEG was not created");
        const cv::Mat image = cv::imread(outputPath.string(), cv::IMREAD_COLOR);
        expect(image.rows == 16 && image.cols == 16, "multiview JPEG size mismatch");
        expectTopRedBottomBlue(image, "multiview JPEG could not be decoded");
    }
}

ElementalMemoryResult makeElementalResult()
{
    ElementalMemoryResult result;
    result.imageCount = 4;
    result.rows = 16;
    result.cols = 16;
    result.targetRows = 2;
    result.targetCols = 2;
    result.imageBytes = static_cast<size_t>(result.rows) * result.cols * 3;
    result.totalBytes = result.imageBytes * result.imageCount;
    result.pixels.reset(new unsigned char[result.totalBytes]);
    result.mode = ElementalMemoryMode::Materialized;
    for (size_t image = 0; image < result.imageCount; ++image) {
        fillRgbImage(result.pixels.get() + image * result.imageBytes,
            result.rows, result.cols, false);
    }
    return result;
}

void testElementalPersistence()
{
    const TempDirectory temp;
    HoloConfig config;
    config.elementalOutDir = temp.path() / "output" / "elemental";
    config.resultTimestamp = "20260716_153045_123";
    config.jpgQuality = 100;
    ElementalMemoryResult result = makeElementalResult();
    ResultSaveReport report;

    persistElementalResult(config, result, report);

    expect(!report.hasWarnings(), "elemental persistence unexpectedly warned");
    const fs::path outputDirectory = timestampedResultDirectory(
        config.elementalOutDir, config.resultTimestamp);
    const std::vector<std::string> names = { "11.jpg", "12.jpg", "21.jpg", "22.jpg" };
    for (const std::string& name : names) {
        const fs::path outputPath = outputDirectory / name;
        expect(fs::exists(outputPath), "elemental JPEG was not created");
        const cv::Mat image = cv::imread(outputPath.string(), cv::IMREAD_COLOR);
        expect(image.rows == 16 && image.cols == 16, "elemental JPEG size mismatch");
        expectTopRedBottomBlue(image, "elemental JPEG could not be decoded");
    }
}

void testElementalFileFallbackNormalizesBgrToRgb()
{
    const TempDirectory temp;
    const fs::path multiviewDirectory = temp.path() / "multiview";
    fs::create_directories(multiviewDirectory);
    const cv::Mat redBgr(16, 16, CV_8UC3, cv::Scalar(0, 0, 255));
    expect(cv::imwrite((multiviewDirectory / "11.jpg").string(), redBgr),
        "file fallback input JPEG could not be written");

    HoloConfig config;
    config.inputMode = PipelineInputMode::Multiview;
    config.inputDirectory = multiviewDirectory;
    config.multiviewOutDir = temp.path() / "generated_multiview_output";
    config.viewRows = 1;
    config.viewCols = 1;
    config.targetRows = 16;
    config.targetCols = 16;
    config.multiviewResolution = 16;
    config.viewNameDigits = 1;
    config.elementalFlipSourceY = false;
    config.elementalFlipViewRows = false;
    config.elementalWriterThreads = 1;
    CliOptions options;
    ElementalMemoryResult result;

    expect(processElemental(config, options, nullptr, &result) == 0,
        "elemental file fallback failed");
    expect(result.hasResult(), "elemental file fallback did not materialize a result");
    expect(result.pixels[0] > 180 && result.pixels[2] < 80,
        "elemental file fallback did not normalize BGR input to RGB memory");
}

void testElementalFileFallbackRejectsNonFirstWrongSize()
{
    const TempDirectory temp;
    const fs::path multiviewDirectory = temp.path() / "multiview";
    fs::create_directories(multiviewDirectory);
    writeSolidImage(multiviewDirectory / "11.jpg", 16, 16);
    writeSolidImage(multiviewDirectory / "12.jpg", 8, 16);

    HoloConfig config;
    config.inputMode = PipelineInputMode::Multiview;
    config.inputDirectory = multiviewDirectory;
    config.viewRows = 1;
    config.viewCols = 2;
    config.targetRows = 16;
    config.targetCols = 16;
    config.multiviewResolution = 16;
    config.viewNameDigits = 1;
    config.elementalWriterThreads = 1;
    CliOptions options;
    ElementalMemoryResult result;

    expect(processElemental(config, options, nullptr, &result) != 0,
        "elemental file fallback must reject a non-first image with the wrong size");
}

void testAllSaveCombinationsOnlyCreateSelectedDirectories()
{
    const TempDirectory temp;
    MeshMemoryResult mesh = makeTriangleMesh();

    MultiviewMemoryResult multiview;
    multiview.plan = std::make_shared<MultiviewRenderPlan>(2, 1, 16);
    multiview.sink = std::make_shared<MemoryFrameSink>(*multiview.plan, true);
    for (std::uint64_t frame = 0; frame < multiview.sink->frameCount(); ++frame) {
        fillRgbImage(multiview.sink->frameData(frame), 16, 16, true);
    }

    ElementalMemoryResult elemental = makeElementalResult();
    for (int combination = 0; combination < 8; ++combination) {
        HoloConfig config;
        config.meshOutDir = temp.path() / "output" / "mesh";
        config.multiviewOutDir = temp.path() / "output" / "multiview";
        config.elementalOutDir = temp.path() / "output" / "elemental";
        config.resultTimestamp = "combination_" + std::to_string(combination);
        config.viewNameDigits = 2;
        config.jpgQuality = 100;
        config.multiviewCamera.captureFlipVertical = true;
        config.saveSettings.mesh = (combination & 1) != 0;
        config.saveSettings.multiview = (combination & 2) != 0;
        config.saveSettings.elemental = (combination & 4) != 0;

        ResultSaveReport report;
        if (config.saveSettings.mesh) {
            persistMeshResult(config, mesh, report);
        }
        if (config.saveSettings.multiview) {
            persistMultiviewResult(config, multiview, report);
        }
        if (config.saveSettings.elemental) {
            persistElementalResult(config, elemental, report);
        }

        expect(!report.hasWarnings(), "a valid save combination produced a warning");
        expect(fs::exists(timestampedResultDirectory(
                   config.meshOutDir, config.resultTimestamp)) == config.saveSettings.mesh,
            "mesh directory selection mismatch");
        expect(fs::exists(timestampedResultDirectory(
                   config.multiviewOutDir, config.resultTimestamp)) == config.saveSettings.multiview,
            "multiview directory selection mismatch");
        expect(fs::exists(timestampedResultDirectory(
                   config.elementalOutDir, config.resultTimestamp)) == config.saveSettings.elemental,
            "elemental directory selection mismatch");
    }
}

void testPersistenceFailureOnlyWarns()
{
    const TempDirectory temp;
    const fs::path blockingFile = temp.path() / "not_a_directory";
    {
        std::ofstream output(blockingFile);
        output << "block directory creation";
    }

    HoloConfig config;
    config.elementalOutDir = blockingFile / "elemental";
    config.resultTimestamp = "20260716_153045_123";
    ElementalMemoryResult result = makeElementalResult();
    const unsigned char firstByte = result.pixels[0];
    ResultSaveReport report;

    persistElementalResult(config, result, report);

    expect(report.hasWarnings(), "persistence failure did not record a warning");
    expect(result.hasResult() && result.pixels[0] == firstByte,
        "persistence failure changed the elemental memory result");
    expect(report.warnings().front().resultType == "elemental",
        "failure warning result type mismatch");
    expect(report.warnings().front().outputDirectory
            == timestampedResultDirectory(config.elementalOutDir, config.resultTimestamp),
        "failure warning directory mismatch");
}

void verifyMultiviewDirectoryWhenRequested()
{
    const QString requestedDirectory = qEnvironmentVariable(
        "MERGEHOLO_MULTIVIEW_VERIFY_DIR");
    if (requestedDirectory.isEmpty()) {
        return;
    }

#ifdef _WIN32
    const fs::path directory(requestedDirectory.toStdWString());
#else
    const fs::path directory(requestedDirectory.toStdString());
#endif
    PipelineInputFiles files;
    std::string error;
    if (!resolvePipelineInput(
            inputSelection(PipelineInputMode::Multiview, directory),
            MultiviewInputSpec{}, &files, &error)) {
        std::cerr << "full multiview verification failed: " << error << std::endl;
        std::exit(1);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    testExternalDepthPointCloudAxisNormalization();
    testPipelineInputModesAndRunPlans();
    testPipelineInputConfigParsing();
    testRgbDepthInputValidation();
    testMeshInputValidation();
    testMultiviewInputValidation();
    testDefaultsAreMemoryOnly();
    testCorrectedMultiviewConventionDoesNotFlipElementalRowsByDefault();
    testExplicitCombinationAndTimestamp();
    testUiTemplateSaveSettings();
    testSaveReportLifecycle();
    testTimestampedDirectory();
    testPipelineMeshInputLoading();
    testMeshPersistence();
    testMeshFileFallbackPersistence();
    testMultiviewPersistence();
    testElementalPersistence();
    testElementalFileFallbackNormalizesBgrToRgb();
    testElementalFileFallbackRejectsNonFirstWrongSize();
    testAllSaveCombinationsOnlyCreateSelectedDirectories();
    testPersistenceFailureOnlyWarns();
    verifyMultiviewDirectoryWhenRequested();
    std::cout << "result persistence tests passed\n";
    return 0;
}
