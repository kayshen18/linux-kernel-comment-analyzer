#include "parser.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    enum class ParserState {
        Normal,
        StringLiteral,
        CharacterLiteral,
        BlockComment
    };

    bool containsNonWhitespace(const std::string& text)
    {
        for (unsigned char character : text) {
            if (!std::isspace(character)) {
                return true;
            }
        }

        return false;
    }

} // namespace

ParseResult CFamilyParser::parse(
    const std::filesystem::path& filePath) const
{
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Unable to open source file: " + filePath.string()
        );
    }

    ParseResult result;
    result.filePath = filePath;

    ParserState state = ParserState::Normal;
    CommentBlock currentBlock;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        ++result.metrics.physicalLines;

        bool hasCode = false;
        bool hasComment = false;
        bool escaped = false;

        if (!containsNonWhitespace(line)) {
            ++result.metrics.blankLines;
            continue;
        }

        std::size_t index = 0;

        // 如果上一行处于块注释中，当前行继续记录。
        if (state == ParserState::BlockComment) {
            currentBlock.text += '\n';
        }

        while (index < line.size()) {
            const char current = line[index];
            const char next =
                index + 1 < line.size()
                ? line[index + 1]
                : '\0';

            if (state == ParserState::BlockComment) {
                hasComment = true;

                currentBlock.text += current;

                if (current == '*' && next == '/') {
                    currentBlock.text += next;
                    currentBlock.endLine = lineNumber;

                    result.comments.push_back(
                        std::move(currentBlock)
                    );

                    currentBlock = CommentBlock{};
                    state = ParserState::Normal;
                    index += 2;
                    continue;
                }

                ++index;
                continue;
            }

            if (state == ParserState::StringLiteral) {
                if (!std::isspace(
                    static_cast<unsigned char>(current))) {
                    hasCode = true;
                }

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    state = ParserState::Normal;
                }

                ++index;
                continue;
            }

            if (state == ParserState::CharacterLiteral) {
                if (!std::isspace(
                    static_cast<unsigned char>(current))) {
                    hasCode = true;
                }

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '\'') {
                    state = ParserState::Normal;
                }

                ++index;
                continue;
            }

            // 从这里开始，状态一定是 Normal。

            if (current == '/' && next == '/') {
                hasComment = true;

                CommentBlock lineComment;
                lineComment.type = CommentType::Line;
                lineComment.startLine = lineNumber;
                lineComment.endLine = lineNumber;
                lineComment.text = line.substr(index);

                result.comments.push_back(
                    std::move(lineComment)
                );

                // // 后面的所有内容都是注释。
                break;
            }

            if (current == '/' && next == '*') {
                hasComment = true;

                currentBlock = CommentBlock{};
                currentBlock.type = CommentType::Block;
                currentBlock.startLine = lineNumber;
                currentBlock.endLine = lineNumber;
                currentBlock.text = "/*";

                state = ParserState::BlockComment;
                index += 2;
                continue;
            }

            if (current == '"') {
                hasCode = true;
                escaped = false;
                state = ParserState::StringLiteral;
                ++index;
                continue;
            }

            if (current == '\'') {
                hasCode = true;
                escaped = false;
                state = ParserState::CharacterLiteral;
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

        /*
         * C/C++ 的普通字符串和字符常量不应自然跨行。
         * 当前版本在行末将这两个状态恢复，避免错误影响后续文件。
         */
        if (state == ParserState::StringLiteral ||
            state == ParserState::CharacterLiteral) {
            state = ParserState::Normal;
        }
    }

    // 即使块注释没有闭合，也保留已经提取的内容。
    if (state == ParserState::BlockComment) {
        currentBlock.endLine = lineNumber;
        result.comments.push_back(std::move(currentBlock));
    }

    return result;
}