#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "ConverPointCloud.h"
#include "FileLibrary.h"
#include "depth_io.h"
#include "modelMoveHandler.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct HoloConfig {
    fs::path depthInputDir;
    fs::path depthConfig;
    fs::path meshConfig;
    fs::path meshObj;

    fs::path outputRoot;
    fs::path multiviewOutDir;
    fs::path elementalOutDir;

    std::string modelType = "obj";
    int multiviewAngle = 30;
    int multiviewPer = 9;
    int multiviewResolution = 150;

    int targetRows = 150;
    int targetCols = 150;
    int viewRows = 270;
    int viewCols = 270;
    int viewNameDigits = 3;
    int jpgQuality = 100;
    int elementalWriterThreads = 0;
    bool elementalFlipSourceY = true;
    bool elementalFlipViewRows = true;
    ModelMoveCameraConfig multiviewCamera;

    bool runDepthPointCloud = true;
    bool runMesh = true;
    bool runTexturedModel = true;
    bool runMultiview = true;
    bool runElemental = true;
};

struct CliOptions {
    fs::path configPath;
    std::string stage = "all";
    bool dryRun = false;
    bool showHelp = false;
};

std::string trim(const std::string& value) {
    const char* whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return {};
    }

    const auto last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parseBool(const std::string& value, bool fallback) {
    const std::string v = lower(trim(value));
    if (v == "1" || v == "true" || v == "yes" || v == "on") {
        return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "off") {
        return false;
    }
    return fallback;
}

int parseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(trim(value));
    }
    catch (...) {
        return fallback;
    }
}

double parseDouble(const std::string& value, double fallback) {
    try {
        return std::stod(trim(value));
    }
    catch (...) {
        return fallback;
    }
}

fs::path resolvePath(const fs::path& configDir, const std::string& value) {
    const std::string cleaned = trim(value);
    if (cleaned.empty()) {
        return {};
    }

    fs::path path(cleaned);
    if (path.is_relative()) {
        path = configDir / path;
    }

    return path.lexically_normal();
}

fs::path resolvePathOrDefault(const fs::path& baseDir, const std::string& value, const std::string& fallback) {
    return resolvePath(baseDir, trim(value).empty() ? fallback : value);
}

std::map<std::string, std::string> readIni(const fs::path& configPath) {
    std::ifstream input(configPath);
    if (!input) {
        throw std::runtime_error("Cannot open config file: " + configPath.string());
    }

    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            continue;
        }

        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        values[lower(trim(line.substr(0, pos)))] = trim(line.substr(pos + 1));
    }

    return values;
}

void applyConfig(HoloConfig& config, const fs::path& configPath) {
    const fs::path configDir = fs::absolute(configPath).parent_path();
    const auto values = readIni(configPath);

    auto get = [&](const std::string& key) -> std::string {
        const auto it = values.find(lower(key));
        return it == values.end() ? std::string{} : it->second;
    };

    config.depthInputDir = resolvePath(configDir, get("depth_input_dir"));
    config.depthConfig = resolvePath(configDir, get("depth_config"));
    config.meshConfig = resolvePath(configDir, get("mesh_config"));
    config.meshObj = resolvePath(configDir, get("mesh_obj"));
    config.outputRoot = resolvePathOrDefault(configDir, get("output_root"), "output");
    config.multiviewOutDir = resolvePathOrDefault(config.outputRoot, get("multiview_out_dir"), "multiview");
    config.elementalOutDir = resolvePathOrDefault(config.outputRoot, get("elemental_out_dir"), "elemental");

    if (!get("model_type").empty()) config.modelType = get("model_type");
    config.multiviewAngle = parseInt(get("multiview_angle"), config.multiviewAngle);
    config.multiviewPer = parseInt(get("multiview_per"), config.multiviewPer);
    config.multiviewResolution = parseInt(get("multiview_resolution"), config.multiviewResolution);
    config.targetRows = parseInt(get("target_rows"), config.targetRows);
    config.targetCols = parseInt(get("target_cols"), config.targetCols);
    config.viewRows = config.multiviewAngle * config.multiviewPer;
    config.viewCols = config.viewRows;
    config.viewNameDigits = parseInt(get("view_name_digits"), config.viewNameDigits);
    config.jpgQuality = parseInt(get("jpg_quality"), config.jpgQuality);
    config.elementalWriterThreads = parseInt(get("elemental_writer_threads"), config.elementalWriterThreads);
    config.elementalFlipSourceY = parseBool(get("elemental_flip_source_y"), config.elementalFlipSourceY);
    config.elementalFlipViewRows = parseBool(get("elemental_flip_view_rows"), config.elementalFlipViewRows);

    config.multiviewCamera.distanceScale = parseDouble(get("multiview_camera_distance_scale"), config.multiviewCamera.distanceScale);
    config.multiviewCamera.centerOffset.set(
        parseDouble(get("multiview_camera_center_offset_x"), config.multiviewCamera.centerOffset.x()),
        parseDouble(get("multiview_camera_center_offset_y"), config.multiviewCamera.centerOffset.y()),
        parseDouble(get("multiview_camera_center_offset_z"), config.multiviewCamera.centerOffset.z()));
    config.multiviewCamera.eyeDirection.set(
        parseDouble(get("multiview_camera_eye_dir_x"), config.multiviewCamera.eyeDirection.x()),
        parseDouble(get("multiview_camera_eye_dir_y"), config.multiviewCamera.eyeDirection.y()),
        parseDouble(get("multiview_camera_eye_dir_z"), config.multiviewCamera.eyeDirection.z()));
    config.multiviewCamera.upDirection.set(
        parseDouble(get("multiview_camera_up_x"), config.multiviewCamera.upDirection.x()),
        parseDouble(get("multiview_camera_up_y"), config.multiviewCamera.upDirection.y()),
        parseDouble(get("multiview_camera_up_z"), config.multiviewCamera.upDirection.z()));
    config.multiviewCamera.fovyDeg = parseDouble(get("multiview_camera_fovy_deg"), config.multiviewCamera.fovyDeg);
    config.multiviewCamera.zNear = parseDouble(get("multiview_camera_z_near"), config.multiviewCamera.zNear);
    config.multiviewCamera.zFar = parseDouble(get("multiview_camera_z_far"), config.multiviewCamera.zFar);
    if (!get("multiview_initial_rotate_x_deg").empty()) {
        config.multiviewCamera.initialRotateXDeg = parseDouble(get("multiview_initial_rotate_x_deg"), config.multiviewCamera.initialRotateXDeg);
        config.multiviewCamera.hasInitialRotateXDeg = true;
    }
    if (!get("multiview_initial_rotate_z_deg").empty()) {
        config.multiviewCamera.initialRotateZDeg = parseDouble(get("multiview_initial_rotate_z_deg"), config.multiviewCamera.initialRotateZDeg);
        config.multiviewCamera.hasInitialRotateZDeg = true;
    }
    config.multiviewCamera.captureFlipVertical = parseBool(get("multiview_capture_flip_vertical"), config.multiviewCamera.captureFlipVertical);

    config.runDepthPointCloud = parseBool(get("run_depth_pointcloud"), config.runDepthPointCloud);
    config.runMesh = parseBool(get("run_mesh"), config.runMesh);
    config.runTexturedModel = parseBool(get("run_textured_model"), config.runTexturedModel);
    config.runMultiview = parseBool(get("run_multiview"), config.runMultiview);
    config.runElemental = parseBool(get("run_elemental"), config.runElemental);
}

bool requireExists(const fs::path& path, const std::string& label) {
    if (!fs::exists(path)) {
        std::cerr << "[error] Missing " << label << ": " << path.string() << std::endl;
        return false;
    }
    return true;
}

std::string padNumber(int value, int digits) {
    std::ostringstream out;
    out << std::setw(digits) << std::setfill('0') << value;
    return out.str();
}

int decimalDigits(int value) {
    int digits = 1;
    while (value >= 10) {
        value /= 10;
        ++digits;
    }
    return digits;
}

fs::path viewPath(const HoloConfig& config, int row, int col) {
    const std::string name = padNumber(row, config.viewNameDigits)
        + padNumber(col, config.viewNameDigits)
        + ".jpg";
    return config.multiviewOutDir / name;
}

fs::path targetPath(const HoloConfig& config, int row, int col) {
    const int rowDigits = decimalDigits(config.targetRows);
    const int colDigits = decimalDigits(config.targetCols);
    const std::string name = padNumber(row, rowDigits) + padNumber(col, colDigits) + ".jpg";
    return config.elementalOutDir / name;
}

fs::path findFirstObj(const fs::path& directory) {
    if (!fs::exists(directory)) {
        return {};
    }

    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (lower(entry.path().extension().string()) == ".obj") {
            return entry.path();
        }
    }

    return {};
}

bool shouldRunStage(const CliOptions& options, const std::string& stageName) {
    return options.stage == "all" || options.stage == stageName;
}

struct StageTiming {
    std::string name;
    double seconds;
    int code;
};

class BoundedIntQueue {
public:
    explicit BoundedIntQueue(size_t capacity)
        : capacity_(std::max<size_t>(1, capacity)) {
    }

    void push(int value) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return closed_ || queue_.size() < capacity_; });
        if (closed_) {
            return;
        }

        queue_.push_back(value);
        notEmpty_.notify_one();
    }

    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }

        value = queue_.front();
        queue_.pop_front();
        notFull_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    const size_t capacity_;
    std::deque<int> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    bool closed_ = false;
};

double elapsedSeconds(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

std::string formatSeconds(double seconds) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << seconds;
    return out.str();
}

std::string formatBytes(size_t bytes) {
    const char* units[] = { "B", "KiB", "MiB", "GiB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        ++unit;
    }

    std::ostringstream out;
    if (unit == 0) {
        out << static_cast<size_t>(value) << " " << units[unit];
    }
    else {
        out << std::fixed << std::setprecision(2) << value << " " << units[unit];
    }
    return out.str();
}

bool checkedMultiply(size_t left, size_t right, size_t& result) {
    if (left != 0 && right > std::numeric_limits<size_t>::max() / left) {
        return false;
    }

    result = left * right;
    return true;
}

int chooseElementalWriterThreads(int requested) {
    if (requested > 0) {
        return std::max(1, requested);
    }

    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    const unsigned int fallback = hardwareThreads == 0 ? 2 : hardwareThreads;
    return static_cast<int>(std::max(1u, std::min(8u, fallback)));
}

template <typename StageRunner>
int runTimedStage(const std::string& name, StageRunner&& runner, std::vector<StageTiming>& timings) {
    const auto start = std::chrono::steady_clock::now();
    const int code = runner();
    const double seconds = elapsedSeconds(start);
    timings.push_back({ name, seconds, code });

    std::cout << "[timing] stage " << name << ": " << formatSeconds(seconds)
              << "s, result=" << code << std::endl;
    return code;
}

void printTimingSummary(const std::vector<StageTiming>& timings) {
    if (timings.empty()) {
        return;
    }

    double total = 0.0;
    for (const StageTiming& timing : timings) {
        total += timing.seconds;
    }

    std::cout << "[timing] summary" << std::endl;
    for (const StageTiming& timing : timings) {
        const double percent = total > 0.0 ? timing.seconds * 100.0 / total : 0.0;
        std::cout << "[timing]   " << std::left << std::setw(10) << timing.name << std::right
                  << " " << formatSeconds(timing.seconds) << "s"
                  << " (" << std::fixed << std::setprecision(1) << percent << "%)"
                  << (timing.code == 0 ? "" : " failed")
                  << std::endl;
    }
    std::cout << "[timing] total measured: " << formatSeconds(total) << "s" << std::endl;
}

int runDepthStage(const HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.depthConfig, "depth_config")) {
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[depth] scan " << config.depthInputDir.string()
                  << " and write *_rgb.ply with " << config.depthConfig.string() << std::endl;
        return 0;
    }

    std::list<std::string> files;
    FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, ".tiff");
    if (files.empty()) {
        std::cerr << "[depth] no .tiff files found in " << config.depthInputDir.string() << std::endl;
        return 1;
    }

    depthImage depth(0);
    int failed = 0;
    for (const std::string& depthFile : files) {
        std::string filename = FileLibrary::getInstance()->getFileNameFromPath(depthFile);
        const size_t extPos = filename.rfind(".tiff");
        if (extPos != std::string::npos) {
            filename = filename.substr(0, extPos);
        }

        const std::string rgbFile = FileLibrary::getInstance()->combineFilePath(
            FileLibrary::getInstance()->getFileParentPath(depthFile), filename + ".jpg");

        std::cout << "[depth] " << depthFile << " + " << rgbFile << std::endl;
        if (!FileLibrary::getInstance()->isFileExists(rgbFile)) {
            std::cerr << "[depth] missing RGB image: " << rgbFile << std::endl;
            ++failed;
            continue;
        }

        if (!depth.depthToPlyColor(depthFile, rgbFile, config.depthConfig.string(), config.depthInputDir.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

int runMeshStage(const HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[mesh] scan " << config.depthInputDir.string()
                  << " and write *_mesh.ply with " << config.meshConfig.string() << std::endl;
        return 0;
    }

    std::list<std::string> files;
    FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, "_rgb.ply");
    if (files.empty()) {
        std::cerr << "[mesh] no *_rgb.ply files found in " << config.depthInputDir.string() << std::endl;
        return 1;
    }

    ConverPointCloud converter;
    int failed = 0;
    for (const std::string& plyFile : files) {
        std::cout << "[mesh] " << plyFile << std::endl;
        if (!converter.meshAPI(plyFile, config.meshConfig.string(), config.depthInputDir.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

int runModelStage(const HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.depthInputDir, "depth_input_dir")
        || !requireExists(config.meshConfig, "mesh_config")) {
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[model] scan " << config.depthInputDir.string()
                  << " and write textured OBJ models" << std::endl;
        return 0;
    }

    std::list<std::string> files;
    FileLibrary::getInstance()->getAllSubFiles(config.depthInputDir.string(), files, false, true, false, "_mesh.ply");
    if (files.empty()) {
        std::cerr << "[model] no *_mesh.ply files found in " << config.depthInputDir.string() << std::endl;
        return 1;
    }

    ConverPointCloud converter;
    int failed = 0;
    for (const std::string& meshFile : files) {
        std::cout << "[model] " << meshFile << std::endl;
        if (!converter.modelAPI(meshFile, config.meshConfig.string())) {
            ++failed;
        }
    }

    return failed == 0 ? 0 : 1;
}

bool setMasterViewerGraphicsContext(osgViewer::Viewer* viewer, float x, float y, int width, int height) {
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->alpha = 1;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc.valid()) {
        std::cerr << "[multiview] graphics context was not created." << std::endl;
        return false;
    }

    double fovy, aspectRatio, zNear, zFar;
    viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);
    const double newAspectRatio = static_cast<double>(traits->width) / static_cast<double>(traits->height);
    const double aspectRatioChange = newAspectRatio / aspectRatio;
    if (aspectRatioChange != 1.0) {
        viewer->getCamera()->getProjectionMatrix() *= osg::Matrix::scale(1.0 / aspectRatioChange, 1.0, 1.0);
    }

    viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
    viewer->getCamera()->setGraphicsContext(gc);
    return true;
}

int runMultiviewStage(HoloConfig& config, const CliOptions& options) {
    if (config.meshObj.empty()) {
        config.meshObj = findFirstObj(config.depthInputDir);
    }

    const int expectedViews = config.multiviewAngle * config.multiviewPer;
    if (expectedViews <= 0 || config.multiviewResolution <= 0) {
        std::cerr << "[multiview] multiview_angle, multiview_per, and multiview_resolution must be positive." << std::endl;
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[multiview] render " << config.meshObj.string()
                  << " to " << config.multiviewOutDir.string()
                  << " as " << expectedViews << "x" << expectedViews
                  << " views, " << config.multiviewResolution << "x"
                  << config.multiviewResolution << " each" << std::endl;
        std::cout << "[multiview] camera distance scale: " << config.multiviewCamera.distanceScale
                  << ", center offset: ("
                  << config.multiviewCamera.centerOffset.x() << ", "
                  << config.multiviewCamera.centerOffset.y() << ", "
                  << config.multiviewCamera.centerOffset.z() << ")"
                  << ", eye dir: ("
                  << config.multiviewCamera.eyeDirection.x() << ", "
                  << config.multiviewCamera.eyeDirection.y() << ", "
                  << config.multiviewCamera.eyeDirection.z() << ")"
                  << ", up: ("
                  << config.multiviewCamera.upDirection.x() << ", "
                  << config.multiviewCamera.upDirection.y() << ", "
                  << config.multiviewCamera.upDirection.z() << ")" << std::endl;
        if (config.multiviewCamera.fovyDeg > 0.0) {
            std::cout << "[multiview] camera fovy: " << config.multiviewCamera.fovyDeg
                      << " deg, zNear: " << config.multiviewCamera.zNear
                      << ", zFar: " << config.multiviewCamera.zFar << std::endl;
        }
        if (!config.meshObj.empty() && !fs::exists(config.meshObj)) {
            std::cout << "[multiview] mesh_obj does not exist yet; it should be created by the model stage." << std::endl;
        }
        return 0;
    }

    if (!requireExists(config.meshObj, "mesh_obj")) {
        return 1;
    }

    if (config.modelType != "obj") {
        std::cerr << "[multiview] integrated renderer currently supports model_type=obj." << std::endl;
        return 1;
    }

    fs::create_directories(config.multiviewOutDir);

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->addEventHandler(new osgViewer::ThreadingHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);
    viewer->addEventHandler(new osgViewer::LODScaleHandler);
    viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);
    viewer->setCameraManipulator(NULL);
    viewer->getCamera()->setClearColor(osg::Vec4f(0.3f, 0.3f, 0.3f, 1.0f));

    if (!setMasterViewerGraphicsContext(viewer.get(), 100, 100, config.multiviewResolution, config.multiviewResolution)) {
        return 1;
    }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    viewer->getCamera()->setPostDrawCallback(new CaptureDrawCallback(image, static_cast<float>(config.multiviewResolution)));

    osg::StateSet* state = viewer->getCamera()->getOrCreateStateSet();
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(config.meshObj.string());
    if (!node.valid()) {
        std::cerr << "[multiview] cannot read OBJ: " << config.meshObj.string() << std::endl;
        return 1;
    }

    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->addChild(node.get());

    std::string outDir = config.multiviewOutDir.string();
    modelMoveHandler* handler = new modelMoveHandler(
        viewer.get(), group.get(), outDir, image.get(), config.modelType,
        static_cast<float>(config.multiviewAngle), static_cast<float>(config.multiviewPer),
        config.multiviewCamera);
    viewer->addEventHandler(handler);

    while (!viewer->done()) {
        viewer->frame();
    }

    return handler->isComplete() ? 0 : 1;
}

int runElementalStage(const HoloConfig& config, const CliOptions& options) {
    if (config.viewRows <= 0 || config.viewCols <= 0 || config.targetRows <= 0 || config.targetCols <= 0) {
        std::cerr << "[elemental] derived view grid and target grid must be positive." << std::endl;
        return 1;
    }

    if (options.dryRun) {
        std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
                  << ", each view should be: " << config.multiviewResolution
                  << "x" << config.multiviewResolution << std::endl;
        std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
                  << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
        std::cout << "[elemental] output dir: " << config.elementalOutDir.string() << std::endl;
        std::cout << "[elemental] flip source Y: " << (config.elementalFlipSourceY ? "true" : "false")
                  << ", flip view rows: " << (config.elementalFlipViewRows ? "true" : "false") << std::endl;
        return 0;
    }

    if (!requireExists(config.multiviewOutDir, "multiview_out_dir")) {
        return 1;
    }

    const fs::path firstView = viewPath(config, 1, 1);
    const fs::path lastView = viewPath(config, config.viewRows, config.viewCols);
    if (!requireExists(firstView, "first multiview image")
        || !requireExists(lastView, "last multiview image")) {
        return 1;
    }

    cv::Mat sample = cv::imread(firstView.string(), cv::IMREAD_COLOR);
    if (sample.empty()) {
        std::cerr << "[error] Cannot read sample image: " << firstView.string() << std::endl;
        return 1;
    }

    if (sample.cols != config.targetCols || sample.rows != config.targetRows) {
        std::cerr << "[warn] View image size is " << sample.cols << "x" << sample.rows
                  << ", target grid is " << config.targetCols << "x" << config.targetRows
                  << ". The target grid should match each view image size." << std::endl;
    }

    size_t viewCount = 0;
    size_t targetPixelCount = 0;
    size_t viewImageBytes = 0;
    size_t totalViewBytes = 0;
    size_t outputImageBytes = 0;
    if (!checkedMultiply(static_cast<size_t>(config.viewRows), static_cast<size_t>(config.viewCols), viewCount)
        || !checkedMultiply(static_cast<size_t>(config.targetRows), static_cast<size_t>(config.targetCols), targetPixelCount)
        || !checkedMultiply(targetPixelCount, static_cast<size_t>(3), viewImageBytes)
        || !checkedMultiply(viewCount, viewImageBytes, totalViewBytes)
        || !checkedMultiply(viewCount, static_cast<size_t>(3), outputImageBytes)) {
        std::cerr << "[elemental] image dimensions are too large." << std::endl;
        return 1;
    }

    if (targetPixelCount > static_cast<size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "[elemental] target image count is too large." << std::endl;
        return 1;
    }

    const int targetPixels = static_cast<int>(targetPixelCount);
    const int writerThreads = std::min(static_cast<int>(targetPixelCount), chooseElementalWriterThreads(config.elementalWriterThreads));
    fs::create_directories(config.elementalOutDir);

    std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
              << ", each view: " << sample.cols << "x" << sample.rows << std::endl;
    std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
              << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
    std::cout << "[elemental] view cache: " << viewCount << " images, "
              << formatBytes(totalViewBytes) << " in memory" << std::endl;
    std::cout << "[elemental] per-writer output buffer: " << formatBytes(outputImageBytes) << std::endl;
    std::cout << "[elemental] writer threads: " << writerThreads << std::endl;
    std::cout << "[elemental] flip source Y: " << (config.elementalFlipSourceY ? "true" : "false")
              << ", flip view rows: " << (config.elementalFlipViewRows ? "true" : "false") << std::endl;

    std::unique_ptr<unsigned char[]> viewPixels;
    try {
        viewPixels.reset(new unsigned char[totalViewBytes]);
    }
    catch (const std::bad_alloc&) {
        std::cerr << "[elemental] cannot allocate view cache: "
                  << formatBytes(totalViewBytes) << std::endl;
        return 1;
    }

    const auto loadStart = std::chrono::steady_clock::now();
    const size_t targetRowBytes = static_cast<size_t>(config.targetCols) * 3;
    int mismatchedViewImages = 0;
    const int loadProgressEvery = std::max(1, config.viewRows / 10);

    for (int viewRow = 1; viewRow <= config.viewRows; ++viewRow) {
        for (int viewCol = 1; viewCol <= config.viewCols; ++viewCol) {
            cv::Mat input;
            if (viewRow == 1 && viewCol == 1) {
                input = sample;
            }
            else {
                const fs::path inputPath = viewPath(config, viewRow, viewCol);
                input = cv::imread(inputPath.string(), cv::IMREAD_COLOR);
                if (input.empty()) {
                    std::cerr << "[error] Cannot read multiview image: " << inputPath.string() << std::endl;
                    return 1;
                }
            }

            const size_t viewIndex = static_cast<size_t>(viewRow - 1) * static_cast<size_t>(config.viewCols)
                + static_cast<size_t>(viewCol - 1);
            unsigned char* viewBase = viewPixels.get() + viewIndex * viewImageBytes;

            const bool mismatchedSize = input.rows != config.targetRows || input.cols != config.targetCols;
            if (mismatchedSize) {
                ++mismatchedViewImages;
            }
            std::fill(viewBase, viewBase + viewImageBytes, 0);
            const int copyCols = std::min(config.targetCols, input.cols);
            const size_t copyBytes = static_cast<size_t>(copyCols) * 3;
            for (int targetRow = 0; targetRow < config.targetRows; ++targetRow) {
                const int sourceRow = config.elementalFlipSourceY
                    ? input.rows - 1 - targetRow
                    : targetRow;
                if (sourceRow < 0 || sourceRow >= input.rows) {
                    continue;
                }
                std::memcpy(
                    viewBase + static_cast<size_t>(targetRow) * targetRowBytes,
                    input.ptr<unsigned char>(sourceRow),
                    copyBytes);
            }
        }

        if (viewRow % loadProgressEvery == 0 || viewRow == config.viewRows) {
            std::cout << "[elemental] loaded view rows " << viewRow << "/"
                      << config.viewRows << std::endl;
        }
    }

    if (mismatchedViewImages > 0) {
        std::cout << "[elemental] warning: " << mismatchedViewImages
                  << " view images did not match target grid "
                  << config.targetCols << "x" << config.targetRows << std::endl;
    }
    std::cout << "[elemental] loaded view cache in "
              << formatSeconds(elapsedSeconds(loadStart)) << "s" << std::endl;

    const std::vector<int> jpgParams = { cv::IMWRITE_JPEG_QUALITY, config.jpgQuality };
    const auto writeStart = std::chrono::steady_clock::now();
    BoundedIntQueue jobs(static_cast<size_t>(std::max(16, writerThreads * 4)));
    std::atomic<int> completedImages(0);
    std::atomic<bool> failed(false);
    std::mutex logMutex;
    std::mutex errorMutex;
    std::string firstError;
    const int progressEvery = std::max(1, targetPixels / 10);

    auto recordError = [&](const std::string& message) {
        if (!failed.exchange(true)) {
            std::lock_guard<std::mutex> lock(errorMutex);
            firstError = message;
        }
    };

    auto writer = [&] {
        cv::Mat output;
        int targetIndex = 0;
        while (jobs.pop(targetIndex)) {
            if (!failed.load()) {
                try {
                    output.create(config.viewRows, config.viewCols, CV_8UC3);
                    const int targetRow = targetIndex / config.targetCols;
                    const int targetCol = targetIndex % config.targetCols;
                    const size_t targetOffset = (static_cast<size_t>(targetRow) * static_cast<size_t>(config.targetCols)
                        + static_cast<size_t>(targetCol)) * 3;

                    for (int outputViewRow = 0; outputViewRow < config.viewRows; ++outputViewRow) {
                        const int sourceViewRow = config.elementalFlipViewRows
                            ? config.viewRows - 1 - outputViewRow
                            : outputViewRow;
                        unsigned char* dst = output.ptr<unsigned char>(outputViewRow);
                        const size_t sourceViewRowOffset = static_cast<size_t>(sourceViewRow)
                            * static_cast<size_t>(config.viewCols);
                        for (int outputViewCol = 0; outputViewCol < config.viewCols; ++outputViewCol) {
                            const size_t viewIndex = sourceViewRowOffset + static_cast<size_t>(outputViewCol);
                            const unsigned char* src = viewPixels.get() + viewIndex * viewImageBytes + targetOffset;
                            dst[0] = src[0];
                            dst[1] = src[1];
                            dst[2] = src[2];
                            dst += 3;
                        }
                    }

                    const int outputRow = targetRow + 1;
                    const int outputCol = targetCol + 1;
                    const fs::path outputPath = targetPath(config, outputRow, outputCol);
                    if (!cv::imwrite(outputPath.string(), output, jpgParams)) {
                        recordError("[error] Cannot write output image: " + outputPath.string());
                    }
                }
                catch (const std::exception& ex) {
                    recordError(std::string("[error] Elemental writer failed: ") + ex.what());
                }
            }

            const int done = ++completedImages;
            if (done % progressEvery == 0 || done == targetPixels) {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "[elemental] wrote " << done << "/" << targetPixels
                          << " images" << std::endl;
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(static_cast<size_t>(writerThreads));
    try {
        for (int i = 0; i < writerThreads; ++i) {
            workers.emplace_back(writer);
        }
    }
    catch (const std::exception& ex) {
        jobs.close();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        std::cerr << "[elemental] cannot start writer thread: " << ex.what() << std::endl;
        return 1;
    }

    for (int targetIndex = 0; targetIndex < targetPixels; ++targetIndex) {
        jobs.push(targetIndex);
    }
    jobs.close();

    for (std::thread& worker : workers) {
        worker.join();
    }

    if (failed.load()) {
        std::lock_guard<std::mutex> lock(errorMutex);
        std::cerr << firstError << std::endl;
        return 1;
    }

    std::cout << "[elemental] wrote output images in "
              << formatSeconds(elapsedSeconds(writeStart)) << "s" << std::endl;
    return 0;
}

void printUsage() {
    std::cout << "Holo pipeline\n"
              << "Usage:\n"
              << "  Holo.exe --config holo_config.ini [--stage all|depth|mesh|model|multiview|elemental] [--dry-run]\n\n"
              << "Default target setup:\n"
              << "  output_root=output => relative output base directory\n"
              << "  multiview_angle=30, multiview_per=9 => 270x270 view images\n"
              << "  multiview_resolution=150 => each view image is 150x150\n"
              << "  target_rows=150, target_cols=150 => 22500 output images\n"
              << "  each output image size is derived from multiview_angle * multiview_per\n";
}

CliOptions parseCli(int argc, char* argv[]) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
        }
        else if (arg == "--dry-run") {
            options.dryRun = true;
        }
        else if (arg == "--config" && i + 1 < argc) {
            options.configPath = argv[++i];
        }
        else if (arg == "--stage" && i + 1 < argc) {
            options.stage = lower(argv[++i]);
        }
    }
    return options;
}

int runPipeline(HoloConfig& config, const CliOptions& options) {
    std::vector<StageTiming> timings;
    auto finish = [&](int code) {
        printTimingSummary(timings);
        return code;
    };

    if (shouldRunStage(options, "depth") && config.runDepthPointCloud) {
        const int code = runTimedStage("depth", [&] { return runDepthStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "mesh") && config.runMesh) {
        const int code = runTimedStage("mesh", [&] { return runMeshStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "model") && config.runTexturedModel) {
        const int code = runTimedStage("model", [&] { return runModelStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "multiview") && config.runMultiview) {
        const int code = runTimedStage("multiview", [&] { return runMultiviewStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    if (shouldRunStage(options, "elemental") && config.runElemental) {
        const int code = runTimedStage("elemental", [&] { return runElementalStage(config, options); }, timings);
        if (code != 0) return finish(code);
    }

    return finish(0);
}

} // namespace

int main(int argc, char* argv[]) {
    const CliOptions options = parseCli(argc, argv);
    if (options.showHelp) {
        printUsage();
        return 0;
    }

    fs::path configPath = options.configPath;
    if (configPath.empty()) {
        configPath = "holo_config.ini";
    }

    if (!fs::exists(configPath)) {
        printUsage();
        std::cerr << "\n[error] Config file not found: " << configPath.string() << std::endl;
        return 1;
    }

    try {
        HoloConfig config;
        applyConfig(config, configPath);
        return runPipeline(config, options);
    }
    catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << std::endl;
        return 1;
    }
}
