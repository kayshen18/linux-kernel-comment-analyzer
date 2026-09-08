#include "parser.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    enum class PythonState {
        Normal,
        SingleQuotedString,
        DoubleQuotedString,
        TripleSingleQuotedString,
        TripleDoubleQuotedString
    };

    bool isWhitespaceOnly(const std::string& line)
    {
        for (unsigned char character : line) {
            if (!std::isspace(character)) {
                return false;
            }
        }

        return true;
    }

    bool beginsWith(
        const std::string& text,
        std::size_t position,
        const std::string& token)
    {
        return text.compare(position, token.size(), token) == 0;
    }

} // namespace

ParseResult PythonParser::parse(
    const std::filesystem::path& filePath) const
{
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Unable to open Python source file: " +
            filePath.string()
        );
    }

    ParseResult result;
    result.filePath = filePath;

    PythonState state = PythonState::Normal;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        ++result.metrics.physicalLines;

        bool hasCode = false;
        bool hasComment = false;
        bool escaped = false;

        if (state == PythonState::Normal &&
            isWhitespaceOnly(line)) {
            ++result.metrics.blankLines;
            continue;
        }

        std::size_t index = 0;

        while (index < line.size()) {
            const char current = line[index];

            if (state ==
                PythonState::TripleSingleQuotedString) {
                hasCode = true;

                if (beginsWith(line, index, "'''")) {
                    state = PythonState::Normal;
                    index += 3;
                }
                else {
                    ++index;
                }

                continue;
            }

            if (state ==
                PythonState::TripleDoubleQuotedString) {
                hasCode = true;

                if (beginsWith(line, index, "\"\"\"")) {
                    state = PythonState::Normal;
                    index += 3;
                }
                else {
                    ++index;
                }

                continue;
            }

            if (state == PythonState::SingleQuotedString) {
                hasCode = true;

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '\'') {
                    state = PythonState::Normal;
                }

                ++index;
                continue;
            }

            if (state == PythonState::DoubleQuotedString) {
                hasCode = true;

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    state = PythonState::Normal;
                }

                ++index;
                continue;
            }

            // 从这里开始，状态为 Normal。

            if (current == '#') {
                hasComment = true;

                CommentBlock comment;
                comment.type = CommentType::Line;
                comment.startLine = lineNumber;
                comment.endLine = lineNumber;
                comment.text = line.substr(index);

                result.comments.push_back(
                    std::move(comment)
                );

                break;
            }

            if (beginsWith(line, index, "'''")) {
                hasCode = true;
                state =
                    PythonState::TripleSingleQuotedString;
                index += 3;
                continue;
            }

            if (beginsWith(line, index, "\"\"\"")) {
                hasCode = true;
                state =
                    PythonState::TripleDoubleQuotedString;
                index += 3;
                continue;
            }

            if (current == '\'') {
                hasCode = true;
                escaped = false;
                state = PythonState::SingleQuotedString;
                ++index;
                continue;
            }

            if (current == '"') {
                hasCode = true;
                escaped = false;
                state = PythonState::DoubleQuotedString;
                ++index;
                continue;
            }

            if (!std::isspace(
                static_cast<unsigned char>(current))) {
                hasCode = true;
            }

            ++index;
        }

        /*
         * 单引号和双引号字符串通常不能直接跨越物理行。
         * 三引号字符串状态则需要保留到后续行。
         */
        if (state == PythonState::SingleQuotedString ||
            state == PythonState::DoubleQuotedString) {
            state = PythonState::Normal;
        }

        /*
         * 三引号字符串中的空行仍属于字符串内容，
         * 因而计作包含代码的物理行。
         */
        if (state ==
            PythonState::TripleSingleQuotedString ||
            state ==
            PythonState::TripleDoubleQuotedString) {
            hasCode = true;
        }

        if (hasCode) {
            ++result.metrics.codeLines;
        }

        if (hasComment) {
            ++result.metrics.commentLines;
        }

        if (hasCode && hasComment) {
            ++result.metrics.mixedLines;
        }
    }

    return result;
}