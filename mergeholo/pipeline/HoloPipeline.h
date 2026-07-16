#pragma once

struct ElementalMemoryResult;
class ResultSaveReport;

int runHoloPipelineCli(int argc, char* argv[]);
int runHoloPipelineCliWithResult(int argc, char* argv[], ElementalMemoryResult* elementalResult);
int runHoloPipelineCliWithResult(
    int argc,
    char* argv[],
    ElementalMemoryResult* elementalResult,
    ResultSaveReport* saveReport);
