#include "parser.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

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

    bool containsText(
        const ParseResult& result,
        const std::string& expectedText)
    {
        for (const CommentBlock& comment : result.comments) {
            if (comment.text.find(expectedText) !=
                std::string::npos) {
                return true;
            }
        }

        return false;
    }

} // namespace

int main()
{
    const std::filesystem::path testFile =
        std::filesystem::path(TEST_FIXTURE_DIR) /
        "example.py";

    PythonParser parser;
    const ParseResult result = parser.parse(testFile);

    expectEqual(
        result.metrics.physicalLines,
        14,
        "Python physical line count"
    );

    expectEqual(
        result.metrics.codeLines,
        10,
        "Python code line count"
    );

    expectEqual(
        result.metrics.commentLines,
        3,
        "Python comment line count"
    );

    expectEqual(
        result.metrics.mixedLines,
        2,
        "Python mixed line count"
    );

    expectEqual(
        result.metrics.blankLines,
        3,
        "Python blank line count"
    );

    expectEqual(
        result.comments.size(),
        3,
        "Python comment block count"
    );

    expectTrue(
        containsText(result, "File-level comment"),
        "extract Python standalone comment"
    );

    expectTrue(
        containsText(result, "Inline comment"),
        "extract Python inline comment"
    );

    expectTrue(
        containsText(result, "Return result"),
        "extract Python return-line comment"
    );

    expectTrue(
        !containsText(result, "#section"),
        "ignore hash inside URL string"
    );

    expectTrue(
        !containsText(result, "this is not a comment"),
        "ignore hash inside quoted string"
    );

    expectTrue(
        !containsText(
            result,
            "Module-style documentation string"
        ),
        "do not classify docstring as hash comment"
    );

    if (failureCount == 0) {
        std::cout << "\nAll Python parser tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " Python parser test(s) failed.\n";

    return EXIT_FAILURE;
}