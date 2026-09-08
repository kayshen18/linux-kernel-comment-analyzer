#include "statistics.h"
#include <filesystem>
#include <string>
#include <system_error>

namespace {

    std::string determineTopLevelDirectory(
        const std::filesystem::path& filePath,
        const std::filesystem::path& sourceRoot)
    {
        std::error_code error;

        const std::filesystem::path relativePath =
            std::filesystem::relative(
                filePath,
                sourceRoot,
                error
            );

        if (error ||
            relativePath.empty() ||
            relativePath.parent_path().empty()) {
            return "(root)";
        }

        return relativePath.begin()->string();
    }

} // namespace

void AggregateMetrics::add(const ParseResult& result)
{
    ++parsedFiles;

    physicalLines += result.metrics.physicalLines;
    codeLines += result.metrics.codeLines;
    commentLines += result.metrics.commentLines;
    mixedLines += result.metrics.mixedLines;
    blankLines += result.metrics.blankLines;
    commentBlocks += result.comments.size();
}

double AggregateMetrics::commentDensity() const
{
    if (physicalLines < blankLines) {
        return 0.0;
    }

    const std::size_t nonBlankLines =
        physicalLines - blankLines;

    if (nonBlankLines == 0) {
        return 0.0;
    }

    return static_cast<double>(commentLines) /
        static_cast<double>(nonBlankLines) *
        100.0;
}

void StatisticsSummary::add(
    const ParseResult& result,
    SourceLanguage language,
    const std::filesystem::path& sourceRoot)
{
    total_.add(result);
    byLanguage_[language].add(result);

    const std::string directory =
        determineTopLevelDirectory(
            result.filePath,
            sourceRoot
        );

    byDirectory_[directory].add(result);
}

void StatisticsSummary::recordFailure()
{
    ++total_.failedFiles;
}

const AggregateMetrics& StatisticsSummary::total() const
{
    return total_;
}

const std::map<SourceLanguage, AggregateMetrics>&
StatisticsSummary::byLanguage() const
{
    return byLanguage_;
}

const std::map<std::string, AggregateMetrics>&
StatisticsSummary::byDirectory() const
{
    return byDirectory_;
}