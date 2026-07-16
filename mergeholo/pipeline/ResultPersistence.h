#pragma once

#include "DepthMeshModelMemory.h"
#include "PipelineContext.h"
#include "ResultSaveSettings.h"

#include <filesystem>
#include <string>

std::filesystem::path timestampedResultDirectory(
    const std::filesystem::path& baseDirectory,
    const std::string& timestamp);

void persistMeshResult(
    const HoloConfig& config,
    const MeshMemoryResult& result,
    ResultSaveReport& report) noexcept;

void persistMultiviewResult(
    const HoloConfig& config,
    const MultiviewMemoryResult& result,
    ResultSaveReport& report) noexcept;

void persistElementalResult(
    const HoloConfig& config,
    const ElementalMemoryResult& result,
    ResultSaveReport& report) noexcept;
