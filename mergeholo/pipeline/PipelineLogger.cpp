#include "PipelineLogger.h"

#include "PipelineTiming.h"
#include "ResultPersistence.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

bool hasSuccessfulStage(const std::vector<StageTiming>& timings, const std::string& name)
{
    for (const StageTiming& timing : timings) {
        if (timing.name == name && timing.code == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

void writePipelineLog(
    const HoloConfig& config,
    const std::vector<StageTiming>& timings,
    const MultiviewMemoryResult& multiviewMemory,
    const ElementalMemoryResult& elementalMemory,
    const ResultSaveReport& saveReport,
    bool depthToMeshMemory,
    bool meshToModelMemory,
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
        log << "\n[input]\n";
        log << "mode=" << pipelineInputModeName(config.inputMode) << "\n";
        log << "directory=" << config.inputDirectory.string() << "\n";
        log << "\n[result_persistence]\n";
        log << "timestamp=" << config.resultTimestamp << "\n";
        log << "mesh.selected=" << (config.saveSettings.mesh ? "true" : "false") << "\n";
        log << "mesh.directory="
            << timestampedResultDirectory(config.meshOutDir, config.resultTimestamp).string() << "\n";
        log << "multiview.selected=" << (config.saveSettings.multiview ? "true" : "false") << "\n";
        log << "multiview.directory="
            << timestampedResultDirectory(config.multiviewOutDir, config.resultTimestamp).string() << "\n";
        log << "elemental.selected=" << (config.saveSettings.elemental ? "true" : "false") << "\n";
        log << "elemental.directory="
            << timestampedResultDirectory(config.elementalOutDir, config.resultTimestamp).string() << "\n";
        log << "warning_count=" << saveReport.warnings().size() << "\n";
        for (size_t index = 0; index < saveReport.warnings().size(); ++index) {
            const ResultSaveWarning& warning = saveReport.warnings()[index];
            log << "warning." << index << ".result_type=" << warning.resultType << "\n";
            log << "warning." << index << ".directory=" << warning.outputDirectory.string() << "\n";
            log << "warning." << index << ".message=" << warning.message << "\n";
        }
        log << "\n[stages]\n";
        for (const StageTiming& timing : timings) {
            log << timing.name << ".seconds=" << formatSeconds(timing.seconds) << "\n";
            log << timing.name << ".result=" << timing.code << "\n";
        }

        log << "\n[pipeline_memory]\n";
        log << "depth_to_mesh=" << (depthToMeshMemory ? "memory" : "file") << "\n";
        const bool memoryMeshMultiview = multiviewMemory.modelSource == "memory_mesh";
        log << "mesh_to_model=" << (meshToModelMemory ? "memory" : "file") << "\n";
        log << "mesh_to_multiview="
            << (memoryMeshMultiview ? "memory" : "file") << "\n";
        log << "depth_cloud_cleared_after_mesh=" << (depthToMeshMemory ? "true" : "not_retained") << "\n";
        if (memoryMeshMultiview) {
            log << "mesh_cleared_after_model=after_multiview\n";
        }
        else {
            log << "mesh_cleared_after_model=" << (meshToModelMemory ? "true" : "not_retained") << "\n";
        }

        if (hasSuccessfulStage(timings, "model")) {
            log << "\n[model]\n";
            log << "mode="
                << (memoryMeshMultiview ? "skipped_for_memory_multiview" : "normal")
                << "\n";
        }

        log << "\n[multiview]\n";
        auto writeModelSource = [&]() {
            if (!multiviewMemory.modelSource.empty()) {
                log << "model_source=" << multiviewMemory.modelSource << "\n";
                log << "model_build_seconds=" << formatSeconds(multiviewMemory.modelBuildSeconds) << "\n";
                log << "model_vertex_count=" << multiviewMemory.modelVertexCount << "\n";
                log << "model_triangle_count=" << multiviewMemory.modelTriangleCount << "\n";
                log << "model_skipped_faces=" << multiviewMemory.modelSkippedFaces << "\n";
            }
        };
        if (multiviewMemory.directAtlasElemental && multiviewMemory.plan) {
            log << "mode=direct-atlas-elemental\n";
            writeModelSource();
            log << "frames=" << multiviewMemory.directFramesCaptured << "\n";
            log << "total_bytes=" << multiviewMemory.directBytesCaptured << "\n";
            log << "total_readable=" << formatBytes(static_cast<size_t>(multiviewMemory.directBytesCaptured)) << "\n";
            log << "resolution=" << multiviewMemory.plan->resolution() << "\n";
            log << "samples_per_axis=" << multiviewMemory.plan->samplesPerAxis() << "\n";
            log << "atlas_pages=" << multiviewMemory.directPagesRendered << "\n";
            log << "atlas_page_readbacks=" << multiviewMemory.directPageReadbacks << "\n";
            log << "atlas_readback_errors=" << multiviewMemory.directReadbackErrors << "\n";
            log << "atlas_render_seconds=" << formatSeconds(multiviewMemory.directRenderSeconds) << "\n";
            log << "atlas_readback_seconds=" << formatSeconds(multiviewMemory.directReadbackSeconds) << "\n";
            log << "atlas_copy_seconds=" << formatSeconds(multiviewMemory.directCopySeconds) << "\n";
            log << "atlas_scatter_seconds=" << formatSeconds(multiviewMemory.directScatterSeconds) << "\n";
            log << "atlas_total_seconds=" << formatSeconds(multiviewMemory.directTotalSeconds) << "\n";
            log << "frame_major_buffer=0\n";
        }
        else if (multiviewMemory.sink && multiviewMemory.plan) {
            log << "mode=atlas-memory\n";
            writeModelSource();
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
            log << "mode=memory\n";
            log << "images=" << elementalMemory.imageCount << "\n";
            log << "image_width=" << elementalMemory.cols << "\n";
            log << "image_height=" << elementalMemory.rows << "\n";
            log << "image_bytes=" << elementalMemory.imageBytes << "\n";
            log << "total_bytes=" << elementalMemory.totalBytes << "\n";
            log << "total_readable=" << formatBytes(elementalMemory.totalBytes) << "\n";
            log << "files_written=0\n";
            log << "materialized=true\n";
        }
        else {
            log << "mode=not_retained\n";
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[log] cannot write pipeline log: " << ex.what() << std::endl;
    }
}
