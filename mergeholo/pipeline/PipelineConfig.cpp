#include "PipelineConfig.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

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

} // namespace

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
    config.logFile = resolvePathOrDefault(config.outputRoot, get("log_file"), "pipeline.log");

    if (!get("model_type").empty()) config.modelType = get("model_type");
    config.multiviewAngle = parseInt(get("multiview_angle"), config.multiviewAngle);
    config.multiviewPer = parseInt(get("multiview_per"), config.multiviewPer);
    config.multiviewResolution = parseInt(get("multiview_resolution"), config.multiviewResolution);
    config.multiviewAtlasSize = parseInt(get("multiview_atlas_size"), config.multiviewAtlasSize);
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

void printUsage() {
    std::cout << "Holo pipeline\n"
              << "Usage:\n"
              << "  Holo.exe --config holo_config.ini [--stage all|depth|mesh|mesh-one|model|multiview|elemental] [--input file] [--dry-run]\n\n"
              << "  mesh processes one PLY. Pass --input when depth_input_dir has multiple *_rgb.ply files.\n\n"
              << "Default target setup:\n"
              << "  output_root=output => relative output base directory\n"
              << "  multiview_angle=90, multiview_per=3 => 270x270 view images\n"
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
        else if (arg == "--input" && i + 1 < argc) {
            options.inputPath = argv[++i];
        }
        else if (arg == "--stage" && i + 1 < argc) {
            options.stage = lower(argv[++i]);
        }
    }
    return options;
}
