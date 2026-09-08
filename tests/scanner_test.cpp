#include "scanner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

    int failureCount = 0;

    void expectEqual(
        std::size_t actual,
        std::size_t expected,
        const std::string& testName)
    {
        if (actual == expected) {
            std::cout << "[PASS] " << testName << '\n';
            return;
        }

        std::cerr
            << "[FAIL] "
            << testName
            << ": expected "
            << expected
            << ", actual "
            << actual
            << '\n';

        ++failureCount;
    }

    void expectTrue(
        bool condition,
        const std::string& testName)
    {
        if (condition) {
            std::cout << "[PASS] " << testName << '\n';
            return;
        }

        std::cerr << "[FAIL] " << testName << '\n';
        ++failureCount;
    }

    bool containsFile(
        const std::vector<SourceFile>& files,
        const std::string& fileName)
    {
        for (const SourceFile& file : files) {
            if (file.path.filename().string() == fileName) {
                return true;
            }
        }

        return false;
    }

} // namespace

int main()
{
    const std::filesystem::path fixtureDirectory =
        TEST_FIXTURE_DIR;

    SourceScanner scanner(fixtureDirectory);
    const std::vector<SourceFile> files = scanner.scan();

    std::map<SourceLanguage, std::size_t> languageCounts;

    for (const SourceFile& file : files) {
        ++languageCounts[file.language];
    }

    expectEqual(
        files.size(),
        6,
        "supported source file count"
    );

    expectEqual(
        languageCounts[SourceLanguage::C],
        2,
        "C file count"
    );

    expectEqual(
        languageCounts[SourceLanguage::Assembly],
        1,
        "assembly file count"
    );

    expectEqual(
        languageCounts[SourceLanguage::Python],
        1,
        "Python file count"
    );

    expectEqual(
        languageCounts[SourceLanguage::Shell],
        1,
        "Shell file count"
    );

    expectEqual(
        languageCounts[SourceLanguage::Makefile],
        1,
        "Makefile count"
    );

    expectTrue(
        containsFile(files, "example.c"),
        "discover example.c"
    );

    expectTrue(
        containsFile(files, "edge_cases.c"),
        "discover edge_cases.c"
    );

    expectTrue(
        containsFile(files, "example.S"),
        "discover uppercase .S assembly file"
    );

    expectTrue(
        containsFile(files, "Makefile"),
        "discover Makefile"
    );

    expectTrue(
        !containsFile(files, "ignored.txt"),
        "ignore unsupported text file"
    );

    if (failureCount == 0) {
        std::cout << "\nAll scanner tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " scanner test(s) failed.\n";

    return EXIT_FAILURE;
}