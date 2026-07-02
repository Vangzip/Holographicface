#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;

namespace {

struct HoloConfig {
    fs::path pointCloudExe;
    fs::path multiviewExe;

    fs::path depthInputDir;
    fs::path depthConfig;
    fs::path meshConfig;
    fs::path meshObj;

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
    int elementalBlockImages = 750;

    bool runDepthPointCloud = true;
    bool runMesh = true;
    bool runTexturedModel = true;
    bool runMultiview = true;
    bool runElemental = true;
    bool multiviewInteractiveStart = true;
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

    config.pointCloudExe = resolvePath(configDir, get("point_cloud_exe"));
    config.multiviewExe = resolvePath(configDir, get("multiview_exe"));
    config.depthInputDir = resolvePath(configDir, get("depth_input_dir"));
    config.depthConfig = resolvePath(configDir, get("depth_config"));
    config.meshConfig = resolvePath(configDir, get("mesh_config"));
    config.meshObj = resolvePath(configDir, get("mesh_obj"));
    config.multiviewOutDir = resolvePath(configDir, get("multiview_out_dir"));
    config.elementalOutDir = resolvePath(configDir, get("elemental_out_dir"));

    if (!get("model_type").empty()) config.modelType = get("model_type");
    config.multiviewAngle = parseInt(get("multiview_angle"), config.multiviewAngle);
    config.multiviewPer = parseInt(get("multiview_per"), config.multiviewPer);
    config.multiviewResolution = parseInt(get("multiview_resolution"), config.multiviewResolution);
    config.targetRows = parseInt(get("target_rows"), config.targetRows);
    config.targetCols = parseInt(get("target_cols"), config.targetCols);
    config.viewRows = parseInt(get("view_rows"), config.viewRows);
    config.viewCols = parseInt(get("view_cols"), config.viewCols);
    config.viewNameDigits = parseInt(get("view_name_digits"), config.viewNameDigits);
    config.jpgQuality = parseInt(get("jpg_quality"), config.jpgQuality);
    config.elementalBlockImages = parseInt(get("elemental_block_images"), config.elementalBlockImages);

    config.runDepthPointCloud = parseBool(get("run_depth_pointcloud"), config.runDepthPointCloud);
    config.runMesh = parseBool(get("run_mesh"), config.runMesh);
    config.runTexturedModel = parseBool(get("run_textured_model"), config.runTexturedModel);
    config.runMultiview = parseBool(get("run_multiview"), config.runMultiview);
    config.runElemental = parseBool(get("run_elemental"), config.runElemental);
    config.multiviewInteractiveStart = parseBool(get("multiview_interactive_start"), config.multiviewInteractiveStart);
}

std::wstring quote(const fs::path& path) {
    return L"\"" + path.wstring() + L"\"";
}

std::wstring quote(const std::wstring& text) {
    return L"\"" + text + L"\"";
}

std::wstring toWide(const std::string& text) {
#ifdef _WIN32
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring(text.begin(), text.end());
    }
    std::wstring wide(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
    return wide;
#else
    return std::wstring(text.begin(), text.end());
#endif
}

int runCommand(const std::wstring& command, bool dryRun) {
    std::wcout << L"[cmd] " << command << std::endl;
    if (dryRun) {
        return 0;
    }
    return _wsystem(command.c_str());
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

int runPointCloudStage(const HoloConfig& config, const CliOptions& options, const std::string& mode) {
    if (!requireExists(config.pointCloudExe, "point_cloud_exe")
        || !requireExists(config.depthInputDir, "depth_input_dir")) {
        return 1;
    }

    const fs::path stageConfig = mode == "-point" ? config.depthConfig : config.meshConfig;
    if (!requireExists(stageConfig, mode == "-point" ? "depth_config" : "mesh_config")) {
        return 1;
    }

    const std::wstring command = quote(config.pointCloudExe)
        + L" " + toWide(mode)
        + L" " + quote(config.depthInputDir)
        + L" -config " + quote(stageConfig);

    return runCommand(command, options.dryRun);
}

int runMultiviewStage(HoloConfig& config, const CliOptions& options) {
    if (!requireExists(config.multiviewExe, "multiview_exe")) {
        return 1;
    }

    if (config.meshObj.empty()) {
        config.meshObj = findFirstObj(config.depthInputDir);
    }

    if (!requireExists(config.meshObj, "mesh_obj")) {
        return 1;
    }

    if (!options.dryRun) {
        fs::create_directories(config.multiviewOutDir);
    }

    const int expectedViews = config.multiviewAngle * config.multiviewPer;
    if (expectedViews != config.viewRows || expectedViews != config.viewCols) {
        std::cerr << "[warn] angle * per = " << expectedViews
                  << ", but view_rows/view_cols = "
                  << config.viewRows << "/" << config.viewCols << std::endl;
    }

    if (config.multiviewInteractiveStart) {
        std::cerr << "[warn] Current multiview executable starts capture after right mouse click." << std::endl;
    }

    const std::wstring command = quote(config.multiviewExe)
        + L" -file " + quote(config.meshObj)
        + L" -type " + toWide(config.modelType)
        + L" -outdir " + quote(config.multiviewOutDir)
        + L" -angle " + std::to_wstring(config.multiviewAngle)
        + L" -per " + std::to_wstring(config.multiviewPer)
        + L" -resolution " + std::to_wstring(config.multiviewResolution);

    return runCommand(command, options.dryRun);
}

int runElementalStage(const HoloConfig& config, const CliOptions& options) {
    if (options.dryRun) {
        std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
                  << ", each view should be: " << config.multiviewResolution
                  << "x" << config.multiviewResolution << std::endl;
        std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
                  << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
        std::cout << "[elemental] output dir: " << config.elementalOutDir.string() << std::endl;
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

    const int targetPixels = config.targetRows * config.targetCols;
    const int blockImages = std::max(1, config.elementalBlockImages);
    fs::create_directories(config.elementalOutDir);

    std::cout << "[elemental] input views: " << config.viewRows << "x" << config.viewCols
              << ", each view: " << sample.cols << "x" << sample.rows << std::endl;
    std::cout << "[elemental] output images: " << config.targetRows << "x" << config.targetCols
              << ", each output: " << config.viewCols << "x" << config.viewRows << std::endl;
    std::cout << "[elemental] block images: " << blockImages << std::endl;

    const std::vector<int> jpgParams = { cv::IMWRITE_JPEG_QUALITY, config.jpgQuality };
    for (int blockStart = 0; blockStart < targetPixels; blockStart += blockImages) {
        const int currentBlock = std::min(blockImages, targetPixels - blockStart);
        std::vector<cv::Mat> outputs;
        outputs.reserve(static_cast<size_t>(currentBlock));
        for (int i = 0; i < currentBlock; ++i) {
            outputs.emplace_back(config.viewRows, config.viewCols, CV_8UC3, cv::Scalar(0, 0, 0));
        }

        for (int viewRow = 1; viewRow <= config.viewRows; ++viewRow) {
            for (int viewCol = 1; viewCol <= config.viewCols; ++viewCol) {
                const fs::path inputPath = viewPath(config, viewRow, viewCol);
                cv::Mat input = cv::imread(inputPath.string(), cv::IMREAD_COLOR);
                if (input.empty()) {
                    std::cerr << "[error] Cannot read multiview image: " << inputPath.string() << std::endl;
                    return 1;
                }

                for (int i = 0; i < currentBlock; ++i) {
                    const int targetIndex = blockStart + i;
                    const int targetRow = targetIndex / config.targetCols;
                    const int targetCol = targetIndex % config.targetCols;
                    if (targetRow >= input.rows || targetCol >= input.cols) {
                        continue;
                    }
                    outputs[static_cast<size_t>(i)].at<cv::Vec3b>(viewRow - 1, viewCol - 1)
                        = input.at<cv::Vec3b>(targetRow, targetCol);
                }
            }
        }

        for (int i = 0; i < currentBlock; ++i) {
            const int targetIndex = blockStart + i;
            const int outputRow = targetIndex / config.targetCols + 1;
            const int outputCol = targetIndex % config.targetCols + 1;
            const fs::path outputPath = targetPath(config, outputRow, outputCol);
            if (!cv::imwrite(outputPath.string(), outputs[static_cast<size_t>(i)], jpgParams)) {
                std::cerr << "[error] Cannot write output image: " << outputPath.string() << std::endl;
                return 1;
            }
        }

        std::cout << "[elemental] wrote " << std::min(blockStart + currentBlock, targetPixels)
                  << "/" << targetPixels << " images" << std::endl;
    }

    return 0;
}

void printUsage() {
    std::cout << "Holo pipeline\n"
              << "Usage:\n"
              << "  Holo.exe --config holo_config.ini [--stage all|depth|mesh|model|multiview|elemental] [--dry-run]\n\n"
              << "Default target setup:\n"
              << "  multiview_angle=30, multiview_per=9 => 270x270 view images\n"
              << "  multiview_resolution=150 => each view image is 150x150\n"
              << "  target_rows=150, target_cols=150 => 22500 output images\n"
              << "  view_rows=270, view_cols=270 => each output image is 270x270\n";
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
    if (shouldRunStage(options, "depth") && config.runDepthPointCloud) {
        const int code = runPointCloudStage(config, options, "-point");
        if (code != 0) return code;
    }

    if (shouldRunStage(options, "mesh") && config.runMesh) {
        const int code = runPointCloudStage(config, options, "-mesh");
        if (code != 0) return code;
    }

    if (shouldRunStage(options, "model") && config.runTexturedModel) {
        const int code = runPointCloudStage(config, options, "-model");
        if (code != 0) return code;
    }

    if (shouldRunStage(options, "multiview") && config.runMultiview) {
        const int code = runMultiviewStage(config, options);
        if (code != 0) return code;
    }

    if (shouldRunStage(options, "elemental") && config.runElemental) {
        const int code = runElementalStage(config, options);
        if (code != 0) return code;
    }

    return 0;
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
