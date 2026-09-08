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
            if (comment.text.find(expectedText) != std::string::npos) {
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

    const std::filesystem::path testFile =
        fixtureDirectory / "example.c";

    CFamilyParser parser;
    const ParseResult result = parser.parse(testFile);

    expectEqual(
        result.metrics.physicalLines,
        18,
        "physical line count"
    );

    expectEqual(
        result.metrics.codeLines,
        9,
        "code line count"
    );

    expectEqual(
        result.metrics.commentLines,
        5,
        "comment line count"
    );

    expectEqual(
        result.metrics.mixedLines,
        1,
        "mixed line count"
    );

    expectEqual(
        result.metrics.blankLines,
        5,
        "blank line count"
    );

    expectEqual(
        result.comments.size(),
        3,
        "comment block count"
    );

    expectTrue(
        containsText(result, "File-level comment"),
        "extract standalone line comment"
    );

    expectTrue(
        containsText(result, "Inline comment"),
        "extract inline comment"
    );

    expectTrue(
        containsText(result, "Multi-line block comment"),
        "extract block comment"
    );

    expectTrue(
        !containsText(result, "https://example.com"),
        "ignore comment symbols inside URL string"
    );

    expectTrue(
        !containsText(result, "this is not a comment"),
        "ignore block-comment symbols inside string"
    );

    std::cout << "\nTesting edge_cases.c:\n";

    const std::filesystem::path edgeCaseFile =
        fixtureDirectory / "edge_cases.c";

    const ParseResult edgeResult = parser.parse(edgeCaseFile);

    expectEqual(
        edgeResult.metrics.physicalLines,
        9,
        "edge cases: physical line count"
    );

    expectEqual(
        edgeResult.metrics.codeLines,
        5,
        "edge cases: code line count"
    );

    expectEqual(
        edgeResult.metrics.commentLines,
        5,
        "edge cases: comment line count"
    );

    expectEqual(
        edgeResult.metrics.mixedLines,
        1,
        "edge cases: mixed line count"
    );

    expectEqual(
        edgeResult.metrics.blankLines,
        0,
        "edge cases: blank line count"
    );

    expectEqual(
        edgeResult.comments.size(),
        5,
        "edge cases: comment block count"
    );

    expectTrue(
        containsText(edgeResult, "inline block"),
        "edge cases: extract inline block comment"
    );

    expectTrue(
        containsText(edgeResult, "first block"),
        "edge cases: extract first block on same line"
    );

    expectTrue(
        containsText(edgeResult, "second block"),
        "edge cases: extract second block on same line"
    );

    expectTrue(
        containsText(edgeResult, "real comment"),
        "edge cases: extract real line comment"
    );

    expectTrue(
        containsText(edgeResult, "multi-line"),
        "edge cases: extract multi-line block comment"
    );

    expectTrue(
        !containsText(edgeResult, "https://example.com"),
        "edge cases: ignore URL inside string"
    );

    expectTrue(
        !containsText(edgeResult, "fake comment"),
        "edge cases: ignore block markers inside string"
    );

    expectTrue(
        !containsText(edgeResult, "still string"),
        "edge cases: handle escaped quote correctly"
    );

    if (failureCount == 0) {
        std::cout << "\nAll parser tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " parser test(s) failed.\n";

    return EXIT_FAILURE;
}