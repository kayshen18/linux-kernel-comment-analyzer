#pragma once

#include "parser.h"
#include "scanner.h"

#include <map>


struct AggregateMetrics {
    std::size_t parsedFiles = 0;
    std::size_t failedFiles = 0;
    std::size_t physicalLines = 0;
    std::size_t codeLines = 0;
    std::size_t commentLines = 0;
    std::size_t mixedLines = 0;
    std::size_t blankLines = 0;
    std::size_t commentBlocks = 0;

    void add(const ParseResult& result);

    double commentDensity() const;
};

class StatisticsSummary {
public:
    void add(
        const ParseResult& result,
        SourceLanguage language,
        const std::filesystem::path& sourceRoot
    );

    void recordFailure();

    const AggregateMetrics& total() const;

    const std::map<SourceLanguage, AggregateMetrics>&
        byLanguage() const;

    const std::map<std::string, AggregateMetrics>&
        byDirectory() const;

private:
    AggregateMetrics total_;

    std::map<SourceLanguage, AggregateMetrics>
        byLanguage_;

    std::map<std::string, AggregateMetrics>
        byDirectory_;
};