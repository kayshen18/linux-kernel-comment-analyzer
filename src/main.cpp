#include "parser.h"
#include "scanner.h"
#include "statistics.h"
#include "writer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct CommandLineOptions {
    std::filesystem::path sourceDirectory = "tests/fixtures";
    std::filesystem::path outputPath = "results/comments.jsonl";
    std::size_t threadCount = 1;
    bool summaryOnly = false;
    bool showHelp = false;
};

std::size_t parseThreadCount(const std::string& value)
{
    std::size_t parsedCharacters = 0;
    const unsigned long parsed = std::stoul(value, &parsedCharacters);

    if (parsedCharacters != value.size() || parsed == 0) {
        throw std::invalid_argument("thread count must be a positive integer");
    }

    return static_cast<std::size_t>(parsed);
}

CommandLineOptions parseCommandLine(int argc, char* argv[])
{
    CommandLineOptions options;

    if (argc >= 2) {
        const std::string firstArgument = argv[1];
        if (firstArgument == "--help" || firstArgument == "-h") {
            options.showHelp = true;
            return options;
        }
        options.sourceDirectory = argv[1];
    }

    bool outputSpecified = false;
    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--summary-only") {
            options.summaryOnly = true;
            continue;
        }

        if (argument == "--threads") {
            if (index + 1 >= argc) {
                throw std::invalid_argument("--threads requires a value");
            }
            options.threadCount = parseThreadCount(argv[++index]);
            continue;
        }

        const std::string prefix = "--threads=";
        if (argument.rfind(prefix, 0) == 0) {
            options.threadCount = parseThreadCount(argument.substr(prefix.size()));
            continue;
        }

        if (!argument.empty() && argument.front() == '-') {
            throw std::invalid_argument("unknown option: " + argument);
        }
        if (outputSpecified) {
            throw std::invalid_argument("more than one output path was provided");
        }

        options.outputPath = argument;
        outputSpecified = true;
    }

    if (options.summaryOnly && outputSpecified) {
        throw std::invalid_argument(
            "an output path cannot be combined with --summary-only"
        );
    }

    return options;
}

void printUsage(const char* executable)
{
    std::cout
        << "Usage:\n"
        << "  " << executable
        << " <source-directory> [output.jsonl] [--threads N]\n"
        << "  " << executable
        << " <source-directory> --summary-only [--threads N]\n\n"
        << "Options:\n"
        << "  --threads N       Number of parser worker threads (default: 1)\n"
        << "  --summary-only    Analyze without writing JSONL output\n"
        << "  --help, -h        Show this help message\n";
}

ParseResult parseSourceFile(const SourceFile& file)
{
    switch (file.language) {
    case SourceLanguage::C:
    case SourceLanguage::Cpp:
        return CFamilyParser{}.parse(file.path);
    case SourceLanguage::Python:
        return PythonParser{}.parse(file.path);
    case SourceLanguage::Shell:
        return ShellParser{}.parse(file.path);
    case SourceLanguage::Makefile:
        return MakefileParser{}.parse(file.path);
    case SourceLanguage::Assembly:
        return AssemblyParser{}.parse(file.path);
    default:
        throw std::invalid_argument("unsupported source language");
    }
}

void printMetrics(const AggregateMetrics& metrics)
{
    std::cout << "  Files:           " << metrics.parsedFiles << '\n';
    std::cout << "  Physical lines:  " << metrics.physicalLines << '\n';
    std::cout << "  Code lines:      " << metrics.codeLines << '\n';
    std::cout << "  Comment lines:   " << metrics.commentLines << '\n';
    std::cout << "  Mixed lines:     " << metrics.mixedLines << '\n';
    std::cout << "  Blank lines:     " << metrics.blankLines << '\n';
    std::cout << "  Comment blocks:  " << metrics.commentBlocks << '\n';
    std::cout
        << "  Comment density: "
        << std::fixed << std::setprecision(2)
        << metrics.commentDensity() << "%\n";
}

} // namespace

int main(int argc, char* argv[])
{
    CommandLineOptions options;
    try {
        options = parseCommandLine(argc, argv);
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        printUsage(argv[0]);
        return 2;
    }

    if (options.showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    std::cout
        << "Source directory: "
        << std::filesystem::absolute(options.sourceDirectory)
        << "\n\n";

    const auto startTime = std::chrono::steady_clock::now();
    SourceScanner scanner(options.sourceDirectory);
    const std::vector<SourceFile> files = scanner.scan();

    std::map<SourceLanguage, std::size_t> discoveredByLanguage;
    for (const SourceFile& file : files) {
        ++discoveredByLanguage[file.language];
    }

    const std::size_t workerCount = files.empty()
        ? 1
        : std::min(options.threadCount, files.size());

    std::cout << "Scan completed.\n";
    std::cout << "Supported files found: " << files.size() << "\n";
    std::cout << "Worker threads: " << workerCount << "\n\n";
    std::cout << "Discovered files by language:\n";
    for (const auto& [language, count] : discoveredByLanguage) {
        std::cout
            << "  " << std::setw(10) << std::left
            << toString(language) << count << '\n';
    }

    StatisticsSummary statistics;
    std::unique_ptr<JsonlWriter> writer;
    try {
        if (!options.summaryOnly) {
            writer = std::make_unique<JsonlWriter>(options.outputPath);
        }
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    std::atomic<std::size_t> nextFile{0};
    std::mutex resultMutex;

    auto worker = [&]() {
        while (true) {
            const std::size_t index = nextFile.fetch_add(
                1,
                std::memory_order_relaxed
            );
            if (index >= files.size()) {
                return;
            }

            const SourceFile& file = files[index];
            try {
                ParseResult result = parseSourceFile(file);
                std::lock_guard<std::mutex> lock(resultMutex);

                if (writer) {
                    writer->write(result, file.language);
                }
                statistics.add(
                    result,
                    file.language,
                    options.sourceDirectory
                );
            }
            catch (const std::exception& error) {
                std::lock_guard<std::mutex> lock(resultMutex);
                statistics.recordFailure();
                std::cerr
                    << "Failed to parse " << file.path
                    << ": " << error.what() << '\n';
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index) {
        workers.emplace_back(worker);
    }
    for (std::thread& thread : workers) {
        thread.join();
    }

    const AggregateMetrics& aggregate = statistics.total();
    const auto endTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = endTime - startTime;

    std::cout << "\nSource analysis summary:\n";
    std::cout << "  Parsed files:    " << aggregate.parsedFiles << '\n';
    std::cout << "  Failed files:    " << aggregate.failedFiles << '\n';
    std::cout << "  Physical lines:  " << aggregate.physicalLines << '\n';
    std::cout << "  Code lines:      " << aggregate.codeLines << '\n';
    std::cout << "  Comment lines:   " << aggregate.commentLines << '\n';
    std::cout << "  Mixed lines:     " << aggregate.mixedLines << '\n';
    std::cout << "  Blank lines:     " << aggregate.blankLines << '\n';
    std::cout << "  Comment blocks:  " << aggregate.commentBlocks << '\n';
    std::cout
        << "  Comment density: "
        << std::fixed << std::setprecision(2)
        << aggregate.commentDensity() << "%\n";
    std::cout
        << "  Elapsed time:    "
        << std::fixed << std::setprecision(3)
        << elapsed.count() << " seconds\n";

    if (writer) {
        std::cout << "  JSONL records:   " << writer->recordsWritten() << '\n';
        std::cout
            << "  Output file:     "
            << std::filesystem::absolute(options.outputPath) << '\n';
    }
    else {
        std::cout << "  Output mode:     summary only\n";
    }

    std::cout << "\nMetrics by language:\n";
    for (const auto& [language, metrics] : statistics.byLanguage()) {
        std::cout << "\n[" << toString(language) << "]\n";
        printMetrics(metrics);
    }

    std::cout << "\nMetrics by top-level directory:\n";
    for (const auto& [directory, metrics] : statistics.byDirectory()) {
        std::cout << "\n[" << directory << "]\n";
        printMetrics(metrics);
    }

    return aggregate.failedFiles == 0 ? 0 : 1;
}
