#include "parser.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    enum class ShellState {
        Normal,
        SingleQuotedString,
        DoubleQuotedString
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

    bool isShellSeparator(char character)
    {
        const unsigned char value =
            static_cast<unsigned char>(character);

        return std::isspace(value) ||
            character == ';' ||
            character == '|' ||
            character == '&' ||
            character == '(' ||
            character == ')' ||
            character == '<' ||
            character == '>';
    }

    bool beginsShellComment(
        const std::string& line,
        std::size_t index,
        std::size_t lineNumber)
    {
        if (line[index] != '#') {
            return false;
        }

        // 第一行的 #! 是解释器指令，不是普通注释。
        if (lineNumber == 1 &&
            index == 0 &&
            line.size() >= 2 &&
            line[1] == '!') {
            return false;
        }

        if (index == 0) {
            return true;
        }

        return isShellSeparator(line[index - 1]);
    }

} // namespace

ParseResult ShellParser::parse(
    const std::filesystem::path& filePath) const
{
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Unable to open Shell source file: " +
            filePath.string()
        );
    }

    ParseResult result;
    result.filePath = filePath;

    ShellState state = ShellState::Normal;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        ++result.metrics.physicalLines;

        bool hasCode = false;
        bool hasComment = false;
        bool escaped = false;

        if (state == ShellState::Normal &&
            isWhitespaceOnly(line)) {
            ++result.metrics.blankLines;
            continue;
        }

        std::size_t index = 0;

        while (index < line.size()) {
            const char current = line[index];

            if (state == ShellState::SingleQuotedString) {
                hasCode = true;

                if (current == '\'') {
                    state = ShellState::Normal;
                }

                ++index;
                continue;
            }

            if (state == ShellState::DoubleQuotedString) {
                hasCode = true;

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    state = ShellState::Normal;
                }

                ++index;
                continue;
            }

            // 从这里开始，状态为 Normal。

            if (beginsShellComment(
                line,
                index,
                lineNumber)) {
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

            if (current == '\'') {
                hasCode = true;
                state = ShellState::SingleQuotedString;
                ++index;
                continue;
            }

            if (current == '"') {
                hasCode = true;
                escaped = false;
                state = ShellState::DoubleQuotedString;
                ++index;
                continue;
            }

            if (!std::isspace(
                static_cast<unsigned char>(current))) {
                hasCode = true;
            }

            ++index;
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