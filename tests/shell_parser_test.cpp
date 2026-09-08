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
        "example.sh";

    ShellParser parser;
    const ParseResult result = parser.parse(testFile);

    expectEqual(
        result.metrics.physicalLines,
        7,
        "Shell physical line count"
    );

    expectEqual(
        result.metrics.codeLines,
        6,
        "Shell code line count"
    );

    expectEqual(
        result.metrics.commentLines,
        2,
        "Shell comment line count"
    );

    expectEqual(
        result.metrics.mixedLines,
        1,
        "Shell mixed line count"
    );

    expectEqual(
        result.metrics.blankLines,
        0,
        "Shell blank line count"
    );

    expectEqual(
        result.comments.size(),
        2,
        "Shell comment block count"
    );

    expectTrue(
        containsText(result, "Shell comment"),
        "extract standalone Shell comment"
    );

    expectTrue(
        containsText(result, "Inline shell comment"),
        "extract inline Shell comment"
    );

    expectTrue(
        !containsText(result, "#!/bin/sh"),
        "do not classify shebang as comment"
    );

    expectTrue(
        !containsText(result, "#fragment"),
        "ignore hash inside URL string"
    );

    expectTrue(
        !containsText(result, "# not a comment"),
        "ignore hash inside quoted string"
    );

    expectTrue(
        !containsText(result, "#suffix"),
        "ignore hash inside shell word"
    );

    if (failureCount == 0) {
        std::cout << "\nAll Shell parser tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " Shell parser test(s) failed.\n";

    return EXIT_FAILURE;
}