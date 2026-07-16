#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct ResultSaveSettings {
    bool mesh = false;
    bool multiview = false;
    bool elemental = false;
};

struct ResultSaveWarning {
    std::string resultType;
    std::filesystem::path outputDirectory;
    std::string message;
};

class ResultSaveReport {
public:
    void addWarning(
        std::string resultType,
        std::filesystem::path outputDirectory,
        std::string message)
    {
        warnings_.push_back(ResultSaveWarning{
            std::move(resultType),
            std::move(outputDirectory),
            std::move(message)
        });
    }

    bool hasWarnings() const
    {
        return !warnings_.empty();
    }

    const std::vector<ResultSaveWarning>& warnings() const
    {
        return warnings_;
    }

    void clear()
    {
        warnings_.clear();
    }

private:
    std::vector<ResultSaveWarning> warnings_;
};
