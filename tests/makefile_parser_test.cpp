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

} // namespace

int main()
{
    const std::filesystem::path testFile =
        std::filesystem::path(TEST_FIXTURE_DIR) /
        "Makefile";

    MakefileParser parser;
    const ParseResult result = parser.parse(testFile);

    expectEqual(result.metrics.physicalLines, 7,
        "Makefile physical lines");

    expectEqual(result.metrics.codeLines, 6,
        "Makefile code lines");

    expectEqual(result.metrics.commentLines, 4,
        "Makefile comment lines");

    expectEqual(result.metrics.mixedLines, 3,
        "Makefile mixed lines");

    expectEqual(result.metrics.blankLines, 0,
        "Makefile blank lines");

    expectEqual(result.comments.size(), 4,
        "Makefile comment blocks");

    expectTrue(
        containsText(result, "Makefile comment"),
        "extract standalone Makefile comment"
    );

    expectTrue(
        containsText(result, "Inline make comment"),
        "extract inline Makefile comment"
    );

    expectTrue(
        containsText(result, "Second make comment"),
        "extract comment without preceding whitespace"
    );

    expectTrue(
        containsText(result, "Recipe comment"),
        "extract recipe Shell comment"
    );

    expectTrue(
        !containsText(result, "#literal"),
        "ignore escaped hash"
    );

    expectTrue(
        !containsText(result, "not a recipe comment"),
        "ignore hash inside recipe string"
    );

    if (failureCount == 0) {
        std::cout
            << "\nAll Makefile parser tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " Makefile parser test(s) failed.\n";

    return EXIT_FAILURE;
}