#include "parser.h"
#include "scanner.h"
#include "statistics.h"
#include "writer.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <string>



int main(int argc, char* argv[])
{
    std::filesystem::path sourceDirectory;

    if (argc >= 2) {
        sourceDirectory = argv[1];
    }
    else {
        sourceDirectory = "tests/fixtures";
    }

    std::filesystem::path outputPath;
    bool summaryOnly = false;

    if (argc >= 3 &&
        std::string(argv[2]) == "--summary-only") {
        summaryOnly = true;
    }
    else if (argc >= 3) {
        outputPath = argv[2];
    }
    else {
        outputPath = "results/comments.jsonl";
    }

    std::cout
        << "Source directory: "
        << std::filesystem::absolute(sourceDirectory)
        << "\n\n";

    const auto startTime =
        std::chrono::steady_clock::now();

    SourceScanner scanner(sourceDirectory);
    const std::vector<SourceFile> files = scanner.scan();

    std::map<SourceLanguage, std::size_t> discoveredByLanguage;

    for (const SourceFile& file : files) {
        ++discoveredByLanguage[file.language];
    }

    std::cout << "Scan completed.\n";
    std::cout << "Supported files found: "
        << files.size()
        << "\n\n";

    std::cout << "Discovered files by language:\n";

    for (const auto& [language, count] : discoveredByLanguage) {
        std::cout
            << "  "
            << std::setw(10)
            << std::left
            << toString(language)
            << count
            << '\n';
    }

    CFamilyParser cFamilyParser;
    PythonParser pythonParser;
    ShellParser shellParser;
    MakefileParser makefileParser;
    StatisticsSummary statistics;
    AssemblyParser assemblyParser;
    std::unique_ptr<JsonlWriter> writer;

    if (!summaryOnly) {
        writer = std::make_unique<JsonlWriter>(outputPath);
    }

    for (const SourceFile& file : files) {
        try {
            ParseResult result;

            switch (file.language) {
            case SourceLanguage::C:
            case SourceLanguage::Cpp:
                result = cFamilyParser.parse(file.path);
                break;

            case SourceLanguage::Python:
                result = pythonParser.parse(file.path);
                break;

            case SourceLanguage::Shell:
                result = shellParser.parse(file.path);
                break;

            case SourceLanguage::Makefile:
                result = makefileParser.parse(file.path);
                break;

            case SourceLanguage::Assembly:
                result = assemblyParser.parse(file.path);
                break;

            default:
                continue;
            }

            statistics.add(
                result,
                file.language,
                sourceDirectory
            );
            if (writer) {
                writer->write(result, file.language);
            }
        }
        catch (const std::exception& error) {
            statistics.recordFailure();

            std::cerr
                << "Failed to parse "
                << file.path
                << ": "
                << error.what()
                << '\n';
        }
    }

    const AggregateMetrics& aggregate =
        statistics.total();

    const auto endTime =
        std::chrono::steady_clock::now();

    const std::chrono::duration<double> elapsed =
        endTime - startTime;

    std::cout << "\nSource analysis summary:\n";
    std::cout << "  Parsed files:    "
        << aggregate.parsedFiles << '\n';
    std::cout << "  Failed files:    "
        << aggregate.failedFiles << '\n';
    std::cout << "  Physical lines:  "
        << aggregate.physicalLines << '\n';
    std::cout << "  Code lines:      "
        << aggregate.codeLines << '\n';
    std::cout << "  Comment lines:   "
        << aggregate.commentLines << '\n';
    std::cout << "  Mixed lines:     "
        << aggregate.mixedLines << '\n';
    std::cout << "  Blank lines:     "
        << aggregate.blankLines << '\n';
    std::cout << "  Comment blocks:  "
        << aggregate.commentBlocks << '\n';

    std::cout
        << "  Comment density: "
        << std::fixed
        << std::setprecision(2)
        << aggregate.commentDensity()
        << "%\n";

    std::cout
        << "  Elapsed time:    "
        << std::fixed
        << std::setprecision(3)
        << elapsed.count()
        << " seconds\n";

    if (writer) {
        std::cout
            << "  JSONL records:   "
            << writer->recordsWritten()
            << '\n';

        std::cout
            << "  Output file:     "
            << std::filesystem::absolute(outputPath)
            << '\n';
    }
    else {
        std::cout
            << "  Output mode:     summary only\n";
    }

    std::cout << "\nMetrics by language:\n";

    for (const auto& [language, metrics] :
        statistics.byLanguage()) {
        std::cout
            << "\n[" << toString(language) << "]\n";

        std::cout
            << "  Files:           "
            << metrics.parsedFiles
            << '\n';

        std::cout
            << "  Physical lines:  "
            << metrics.physicalLines
            << '\n';

        std::cout
            << "  Code lines:      "
            << metrics.codeLines
            << '\n';

        std::cout
            << "  Comment lines:   "
            << metrics.commentLines
            << '\n';

        std::cout
            << "  Mixed lines:     "
            << metrics.mixedLines
            << '\n';

        std::cout
            << "  Blank lines:     "
            << metrics.blankLines
            << '\n';

        std::cout
            << "  Comment blocks:  "
            << metrics.commentBlocks
            << '\n';

        std::cout
            << "  Comment density: "
            << std::fixed
            << std::setprecision(2)
            << metrics.commentDensity()
            << "%\n";
    }

    std::cout << "\nMetrics by top-level directory:\n";

    for (const auto& [directory, metrics] :
        statistics.byDirectory()) {
        std::cout
            << "\n[" << directory << "]\n";

        std::cout
            << "  Files:           "
            << metrics.parsedFiles
            << '\n';

        std::cout
            << "  Physical lines:  "
            << metrics.physicalLines
            << '\n';

        std::cout
            << "  Code lines:      "
            << metrics.codeLines
            << '\n';

        std::cout
            << "  Comment lines:   "
            << metrics.commentLines
            << '\n';

        std::cout
            << "  Mixed lines:     "
            << metrics.mixedLines
            << '\n';

        std::cout
            << "  Blank lines:     "
            << metrics.blankLines
            << '\n';

        std::cout
            << "  Comment blocks:  "
            << metrics.commentBlocks
            << '\n';

        std::cout
            << "  Comment density: "
            << std::fixed
            << std::setprecision(2)
            << metrics.commentDensity()
            << "%\n";
    }

    return aggregate.failedFiles == 0 ? 0 : 1;
}