#pragma once

#include "PipelineContext.h"

#include <filesystem>

void applyConfig(HoloConfig& config, const std::filesystem::path& configPath);
CliOptions parseCli(int argc, char* argv[]);
void printUsage();
