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
        const std::string& name)
    {
        if (actual == expected) {
            std::cout << "[PASS] " << name << '\n';
            return;
        }

        std::cerr
            << "[FAIL] " << name
            << ": expected " << expected
            << ", actual " << actual << '\n';

        ++failureCount;
    }

    void expectTrue(
        bool condition,
        const std::string& name)
    {
        if (condition) {
            std::cout << "[PASS] " << name << '\n';
            return;
        }

        std::cerr << "[FAIL] " << name << '\n';
        ++failureCount;
    }

    bool containsText(
        const ParseResult& result,
        const std::string& text)
    {
        for (const CommentBlock& comment : result.comments) {
            if (comment.text.find(text) !=
                std::string::npos) {
                return true;
            }
        }

        return false;
    }

} // namespace

int main()
{
    const auto testFile =
        std::filesystem::path(TEST_FIXTURE_DIR) /
        "example.S";

    AssemblyParser parser;
    const ParseResult result = parser.parse(testFile);

    expectEqual(result.metrics.physicalLines, 8,
        "assembly physical lines");
    expectEqual(result.metrics.codeLines, 4,
        "assembly code lines");
    expectEqual(result.metrics.commentLines, 5,
        "assembly comment lines");
    expectEqual(result.metrics.mixedLines, 1,
        "assembly mixed lines");
    expectEqual(result.metrics.blankLines, 0,
        "assembly blank lines");
    expectEqual(result.comments.size(), 3,
        "assembly comment blocks");

    expectTrue(
        containsText(result, "Standalone assembly comment"),
        "extract standalone assembly comment"
    );

    expectTrue(
        containsText(result, "Inline assembly comment"),
        "extract inline assembly comment"
    );

    expectTrue(
        containsText(result, "Block assembly comment"),
        "extract block assembly comment"
    );

    expectTrue(
        !containsText(result, "#define"),
        "ignore preprocessor directive"
    );

    expectTrue(
        !containsText(result, "# not a comment"),
        "ignore hash inside assembly string"
    );

    if (failureCount == 0) {
        std::cout
            << "\nAll assembly parser tests passed.\n";

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}