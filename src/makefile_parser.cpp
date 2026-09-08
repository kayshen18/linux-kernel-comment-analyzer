#include "parser.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    bool isWhitespaceOnly(const std::string& line)
    {
        for (unsigned char character : line) {
            if (!std::isspace(character)) {
                return false;
            }
        }

        return true;
    }

    bool isEscaped(
        const std::string& line,
        std::size_t position)
    {
        std::size_t backslashCount = 0;

        while (position > 0 &&
            line[position - 1] == '\\') {
            ++backslashCount;
            --position;
        }

        return backslashCount % 2 == 1;
    }

    bool isShellSeparator(char character)
    {
        return std::isspace(
            static_cast<unsigned char>(character)) ||
            character == ';' ||
            character == '|' ||
            character == '&' ||
            character == '(' ||
            character == ')' ||
            character == '<' ||
            character == '>';
    }

    std::size_t findMakeComment(
        const std::string& line)
    {
        for (std::size_t index = 0;
            index < line.size();
            ++index) {
            if (line[index] == '#' &&
                !isEscaped(line, index)) {
                return index;
            }
        }

        return std::string::npos;
    }

    std::size_t findRecipeComment(
        const std::string& line)
    {
        bool inSingleQuote = false;
        bool inDoubleQuote = false;
        bool escaped = false;

        for (std::size_t index = 0;
            index < line.size();
            ++index) {
            const char current = line[index];

            if (inSingleQuote) {
                if (current == '\'') {
                    inSingleQuote = false;
                }

                continue;
            }

            if (inDoubleQuote) {
                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    inDoubleQuote = false;
                }

                continue;
            }

            if (current == '\'') {
                inSingleQuote = true;
                continue;
            }

            if (current == '"') {
                inDoubleQuote = true;
                continue;
            }

            if (current != '#') {
                continue;
            }

            if (index == 0 ||
                isShellSeparator(line[index - 1])) {
                return index;
            }
        }

        return std::string::npos;
    }

} // namespace

ParseResult MakefileParser::parse(
    const std::filesystem::path& filePath) const
{
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Unable to open Makefile: " +
            filePath.string()
        );
    }

    ParseResult result;
    result.filePath = filePath;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        ++result.metrics.physicalLines;

        if (isWhitespaceOnly(line)) {
            ++result.metrics.blankLines;
            continue;
        }

        const bool isRecipe =
            !line.empty() && line.front() == '\t';

        const std::size_t commentPosition =
            isRecipe
            ? findRecipeComment(line)
            : findMakeComment(line);

        bool hasComment =
            commentPosition != std::string::npos;

        bool hasCode = false;

        const std::size_t codeEnd =
            hasComment
            ? commentPosition
            : line.size();

        for (std::size_t index = 0;
            index < codeEnd;
            ++index) {
            if (!std::isspace(
                static_cast<unsigned char>(line[index]))) {
                hasCode = true;
                break;
            }
        }

        if (hasComment) {
            CommentBlock comment;
            comment.type = CommentType::Line;
            comment.startLine = lineNumber;
            comment.endLine = lineNumber;
            comment.text = line.substr(commentPosition);

            result.comments.push_back(
                std::move(comment)
            );
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