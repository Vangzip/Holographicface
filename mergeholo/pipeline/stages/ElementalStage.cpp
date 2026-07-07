#include "ElementalStage.h"

#include "ElementalProcessor.h"

int runElementalStage(
    const HoloConfig& config,
    const CliOptions& options,
    const MultiviewMemoryResult* memoryResult,
    ElementalMemoryResult* elementalResult)
{
    return processElemental(config, options, memoryResult, elementalResult);
}
