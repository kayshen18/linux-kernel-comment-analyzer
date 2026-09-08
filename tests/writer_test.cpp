#include "writer.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

    int failureCount = 0;

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

} // namespace

int main()
{
    const std::filesystem::path outputPath =
        std::filesystem::temp_directory_path() /
        "linux_comment_analyzer_writer_test.jsonl";

    ParseResult result;
    result.filePath = "sample.c";

    CommentBlock comment;
    comment.type = CommentType::Line;
    comment.startLine = 7;
    comment.endLine = 7;
    comment.text =
        "// quote: \" path: C:\\tmp\nnext";

    result.comments.push_back(comment);

    std::size_t recordsWritten = 0;

    // 离开作用域后关闭输出文件。
    {
        JsonlWriter writer(outputPath);
        writer.write(result, SourceLanguage::C);
        recordsWritten = writer.recordsWritten();
    }

    expectEqual(
        recordsWritten,
        1,
        "writer record count"
    );

    expectTrue(
        std::filesystem::exists(outputPath),
        "output file created"
    );

    std::ifstream input(outputPath);
    std::string jsonLine;
    std::getline(input, jsonLine);

    expectTrue(
        !jsonLine.empty(),
        "JSONL record is not empty"
    );

    expectTrue(
        jsonLine.find("\"file\":\"sample.c\"") !=
        std::string::npos,
        "write file field"
    );

    expectTrue(
        jsonLine.find("\"language\":\"C\"") !=
        std::string::npos,
        "write language field"
    );

    expectTrue(
        jsonLine.find("\"type\":\"line\"") !=
        std::string::npos,
        "write comment type"
    );

    expectTrue(
        jsonLine.find("\"start_line\":7") !=
        std::string::npos,
        "write start line"
    );

    expectTrue(
        jsonLine.find("\"end_line\":7") !=
        std::string::npos,
        "write end line"
    );

    const std::string expectedText =
        "\"text\":\"// quote: \\\" path: C:\\\\tmp\\nnext\"";

    expectTrue(
        jsonLine.find(expectedText) != std::string::npos,
        "escape quote, backslash and newline"
    );

    input.close();

    std::error_code error;
    std::filesystem::remove(outputPath, error);

    expectTrue(
        !std::filesystem::exists(outputPath),
        "remove temporary output file"
    );

    if (failureCount == 0) {
        std::cout << "\nAll writer tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " writer test(s) failed.\n";

    return EXIT_FAILURE;
}