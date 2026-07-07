#include "PipelineTiming.h"

double totalTimingSeconds(const std::vector<StageTiming>& timings)
{
    double total = 0.0;
    for (const StageTiming& timing : timings) {
        total += timing.seconds;
    }
    return total;
}

void printTimingSummary(const std::vector<StageTiming>& timings)
{
    if (timings.empty()) {
        return;
    }

    const double total = totalTimingSeconds(timings);

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
