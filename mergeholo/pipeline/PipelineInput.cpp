#include "PipelineInput.h"

#include "DepthMeshModelMemory.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include <pcl/PCLPointField.h>
#include <pcl/io/ply_io.h>

namespace {

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool endsWith(const std::string& value, const std::string& suffix)
{
    return value.size() >= suffix.size()
        && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool hasVertexColorField(const pcl::PCLPointCloud2& cloud)
{
    for (const pcl::PCLPointField& field : cloud.fields) {
        if (field.name == "rgb" || field.name == "rgba") {
            return true;
        }
    }
    return false;
}

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

std::string padNumber(int value, int digits)
{
    std::ostringstream output;
    output << std::setw(digits) << std::setfill('0') << value;
    return output.str();
}

bool requireInputDirectory(
    const std::filesystem::path& directory,
    std::string* errorMessage)
{
    std::error_code error;
    if (directory.empty() || !std::filesystem::is_directory(directory, error)) {
        setError(errorMessage, "Input directory does not exist: " + directory.string());
        return false;
    }
    return true;
}

bool resolveRgbDepth(
    const std::filesystem::path& directory,
    PipelineInputFiles* files,
    std::string* errorMessage)
{
    std::vector<std::filesystem::path> depthFiles;
    for (const std::filesystem::directory_entry& entry
        : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension().string() == ".tiff") {
            depthFiles.push_back(entry.path());
        }
    }

    if (depthFiles.size() != 1) {
        setError(errorMessage,
            "RGB/depth input must contain exactly one .tiff file; found "
                + std::to_string(depthFiles.size()) + ".");
        return false;
    }

    std::filesystem::path rgbPath = depthFiles.front();
    rgbPath.replace_extension(".jpg");
    if (!std::filesystem::is_regular_file(rgbPath)) {
        setError(errorMessage,
            "Missing same-basename RGB image: " + rgbPath.string());
        return false;
    }

    files->depthPath = depthFiles.front().lexically_normal();
    files->rgbPath = rgbPath.lexically_normal();
    return true;
}

bool resolveMesh(
    const std::filesystem::path& directory,
    PipelineInputFiles* files,
    std::string* errorMessage)
{
    std::vector<std::filesystem::path> meshFiles;
    for (const std::filesystem::directory_entry& entry
        : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        if (endsWith(fileName, "_mesh.ply")) {
            meshFiles.push_back(entry.path());
        }
    }

    if (meshFiles.size() != 1) {
        setError(errorMessage,
            "Mesh input must contain exactly one *_mesh.ply file; found "
                + std::to_string(meshFiles.size()) + ".");
        return false;
    }

    std::string baseName = meshFiles.front().stem().string();
    if (baseName.size() <= 5) {
        setError(errorMessage, "Mesh input filename must end with _mesh.ply.");
        return false;
    }
    baseName.resize(baseName.size() - 5);
    const std::filesystem::path rgbPath = directory / (baseName + ".jpg");
    if (!std::filesystem::is_regular_file(rgbPath)) {
        setError(errorMessage,
            "Missing mesh texture image: " + rgbPath.string());
        return false;
    }
    if (cv::imread(rgbPath.string(), cv::IMREAD_COLOR).empty()) {
        setError(errorMessage,
            "Cannot read mesh companion image: " + rgbPath.string());
        return false;
    }

    files->meshPath = meshFiles.front().lexically_normal();
    files->rgbPath = rgbPath.lexically_normal();
    return true;
}

bool resolveMultiview(
    const std::filesystem::path& directory,
    const MultiviewInputSpec& spec,
    PipelineInputFiles* files,
    std::string* errorMessage)
{
    if (spec.viewRows <= 0 || spec.viewCols <= 0 || spec.viewNameDigits <= 0
        || spec.viewWidth <= 0 || spec.viewHeight <= 0) {
        setError(errorMessage, "Multiview input dimensions must be positive.");
        return false;
    }

    std::filesystem::path firstView;
    for (int row = 1; row <= spec.viewRows; ++row) {
        for (int col = 1; col <= spec.viewCols; ++col) {
            const std::filesystem::path viewPath = directory
                / (padNumber(row, spec.viewNameDigits)
                    + padNumber(col, spec.viewNameDigits) + ".jpg");
            if (!std::filesystem::is_regular_file(viewPath)) {
                setError(errorMessage,
                    "Missing multiview image: " + viewPath.string());
                return false;
            }
            if (firstView.empty()) {
                firstView = viewPath;
            }
        }
    }

    const cv::Mat sample = cv::imread(firstView.string(), cv::IMREAD_COLOR);
    if (sample.empty()) {
        setError(errorMessage,
            "Cannot read multiview sample image: " + firstView.string());
        return false;
    }
    if (sample.cols != spec.viewWidth || sample.rows != spec.viewHeight) {
        setError(errorMessage,
            "Multiview image size is " + std::to_string(sample.cols) + "x"
                + std::to_string(sample.rows) + "; expected "
                + std::to_string(spec.viewWidth) + "x"
                + std::to_string(spec.viewHeight) + ".");
        return false;
    }

    files->multiviewDirectory = directory.lexically_normal();
    return true;
}

} // namespace

std::string pipelineInputModeName(PipelineInputMode mode)
{
    switch (mode) {
    case PipelineInputMode::RgbDepth:
        return "rgb_depth";
    case PipelineInputMode::Mesh:
        return "mesh";
    case PipelineInputMode::Multiview:
        return "multiview";
    case PipelineInputMode::Camera:
    default:
        return "camera";
    }
}

bool parsePipelineInputMode(const std::string& value, PipelineInputMode* mode)
{
    if (!mode) {
        return false;
    }

    const std::string normalized = lower(value);
    if (normalized.empty() || normalized == "camera" || normalized == "none") {
        *mode = PipelineInputMode::Camera;
        return true;
    }
    if (normalized == "rgb_depth" || normalized == "rgb-depth" || normalized == "rgbdepth") {
        *mode = PipelineInputMode::RgbDepth;
        return true;
    }
    if (normalized == "mesh") {
        *mode = PipelineInputMode::Mesh;
        return true;
    }
    if (normalized == "multiview") {
        *mode = PipelineInputMode::Multiview;
        return true;
    }
    return false;
}

PipelineRunPlan makePipelineRunPlan(PipelineInputMode mode)
{
    PipelineRunPlan plan;
    plan.readOnlySource = mode != PipelineInputMode::Camera;
    switch (mode) {
    case PipelineInputMode::Mesh:
        plan.depth = false;
        plan.mesh = false;
        plan.model = false;
        plan.preloadMesh = true;
        break;
    case PipelineInputMode::Multiview:
        plan.depth = false;
        plan.mesh = false;
        plan.model = false;
        plan.multiview = false;
        plan.useFileMultiview = true;
        break;
    case PipelineInputMode::RgbDepth:
    case PipelineInputMode::Camera:
    default:
        break;
    }
    return plan;
}

bool resolvePipelineInput(
    const PipelineInputSelection& selection,
    const MultiviewInputSpec& multiviewSpec,
    PipelineInputFiles* files,
    std::string* errorMessage)
{
    if (!files) {
        setError(errorMessage, "Internal input result is not available.");
        return false;
    }

    *files = PipelineInputFiles{};
    if (errorMessage) {
        errorMessage->clear();
    }
    if (selection.mode == PipelineInputMode::Camera) {
        return true;
    }
    if (!requireInputDirectory(selection.directory, errorMessage)) {
        return false;
    }

    try {
        switch (selection.mode) {
        case PipelineInputMode::RgbDepth:
            return resolveRgbDepth(selection.directory, files, errorMessage);
        case PipelineInputMode::Mesh:
            return resolveMesh(selection.directory, files, errorMessage);
        case PipelineInputMode::Multiview:
            return resolveMultiview(selection.directory, multiviewSpec, files, errorMessage);
        case PipelineInputMode::Camera:
        default:
            return true;
        }
    }
    catch (const std::filesystem::filesystem_error& error) {
        *files = PipelineInputFiles{};
        setError(errorMessage, "Cannot inspect input directory: " + std::string(error.what()));
        return false;
    }
}

bool loadPipelineMeshInput(
    const PipelineInputFiles& files,
    MeshMemoryResult* result,
    std::string* errorMessage)
{
    if (!result) {
        setError(errorMessage, "Internal mesh input result is not available.");
        return false;
    }
    result->clear();
    if (errorMessage) {
        errorMessage->clear();
    }
    if (files.meshPath.empty() || files.rgbPath.empty()) {
        setError(errorMessage, "Mesh input paths are incomplete.");
        return false;
    }

    pcl::PolygonMesh::Ptr mesh(new pcl::PolygonMesh);
    if (pcl::io::loadPLYFile(files.meshPath.string(), *mesh) != 0
        || mesh->cloud.data.empty() || mesh->polygons.empty()) {
        setError(errorMessage, "Cannot load polygon mesh: " + files.meshPath.string());
        return false;
    }
    if (!hasVertexColorField(mesh->cloud)) {
        setError(errorMessage,
            "Mesh input must contain RGB or RGBA vertex colors: "
                + files.meshPath.string());
        return false;
    }
    if (cv::imread(files.rgbPath.string(), cv::IMREAD_COLOR).empty()) {
        setError(errorMessage,
            "Cannot read mesh companion image: " + files.rgbPath.string());
        return false;
    }

    std::string baseName = files.meshPath.stem().string();
    const std::string suffix = "_mesh";
    if (endsWith(lower(baseName), suffix)) {
        baseName.resize(baseName.size() - suffix.size());
    }

    result->baseName = baseName;
    result->meshPath = files.meshPath;
    result->rgbPath = files.rgbPath;
    result->mesh = std::move(mesh);
    return true;
}
