#include "PipelineLogger.h"

#include "PipelineTiming.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

void writePipelineLog(
    const HoloConfig& config,
    const std::vector<StageTiming>& timings,
    const MultiviewMemoryResult& multiviewMemory,
    const ElementalMemoryResult& elementalMemory,
    int resultCode,
    double wallSeconds)
{
    try {
        fs::create_directories(config.logFile.parent_path());
        std::ofstream log(config.logFile, std::ios::out | std::ios::trunc);
        if (!log) {
            std::cerr << "[log] cannot write pipeline log: " << config.logFile.string() << std::endl;
            return;
        }

        log << "MergeHolo pipeline log\n";
        log << "result_code=" << resultCode << "\n";
        log << "wall_seconds=" << formatSeconds(wallSeconds) << "\n";
        log << "measured_stage_seconds=" << formatSeconds(totalTimingSeconds(timings)) << "\n";
        log << "\n[stages]\n";
        for (const StageTiming& timing : timings) {
            log << timing.name << ".seconds=" << formatSeconds(timing.seconds) << "\n";
            log << timing.name << ".result=" << timing.code << "\n";
        }

        log << "\n[multiview]\n";
        if (multiviewMemory.sink && multiviewMemory.plan) {
            log << "mode=atlas-memory\n";
            log << "frames=" << multiviewMemory.sink->frameCount() << "\n";
            log << "frame_bytes=" << multiviewMemory.sink->frameBytes() << "\n";
            log << "total_bytes=" << multiviewMemory.sink->totalBytes() << "\n";
            log << "total_readable=" << formatBytes(static_cast<size_t>(multiviewMemory.sink->totalBytes())) << "\n";
            log << "resolution=" << multiviewMemory.plan->resolution() << "\n";
            log << "samples_per_axis=" << multiviewMemory.plan->samplesPerAxis() << "\n";
        }
        else {
            log << "mode=not_retained\n";
        }

        log << "\n[elemental]\n";
        if (elementalMemory.hasResult()) {
            log << "mode="
                << (elementalMemory.isVirtual() ? "virtual_from_multiview" : "memory")
                << "\n";
            log << "images=" << elementalMemory.imageCount << "\n";
            log << "image_width=" << elementalMemory.cols << "\n";
            log << "image_height=" << elementalMemory.rows << "\n";
            log << "image_bytes=" << elementalMemory.imageBytes << "\n";
            log << "total_bytes=" << elementalMemory.totalBytes << "\n";
            log << "total_readable=" << formatBytes(elementalMemory.totalBytes) << "\n";
            log << "files_written=0\n";
            log << "materialized=" << (elementalMemory.isMaterialized() ? "true" : "false") << "\n";
        }
        else {
            log << "mode=not_retained\n";
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[log] cannot write pipeline log: " << ex.what() << std::endl;
    }
}
