#include "parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    enum class AssemblyState {
        Normal,
        StringLiteral,
        CharacterLiteral,
        BlockComment
    };

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {
                return static_cast<char>(
                    std::tolower(character)
                    );
            }
        );

        return value;
    }

    bool isWhitespaceOnly(const std::string& line)
    {
        for (unsigned char character : line) {
            if (!std::isspace(character)) {
                return false;
            }
        }

        return true;
    }

    std::string detectArchitecture(
        const std::filesystem::path& filePath)
    {
        const std::string path =
            toLower(filePath.generic_string());

        const std::string marker = "/arch/";
        const std::size_t position = path.find(marker);

        if (position == std::string::npos) {
            return "unknown";
        }

        const std::size_t start =
            position + marker.size();

        const std::size_t end =
            path.find('/', start);

        if (end == std::string::npos) {
            return path.substr(start);
        }

        return path.substr(start, end - start);
    }

    bool supportsHashComment(
        const std::string& architecture)
    {
        static const std::set<std::string> architectures = {
            "alpha",
            "arc",
            "loongarch",
            "mips",
            "parisc",
            "powerpc",
            "riscv",
            "s390",
            "x86",
            "unknown"
        };

        return architectures.count(architecture) != 0;
    }

    bool supportsAtComment(
        const std::string& architecture)
    {
        return architecture == "arm";
    }

    bool isPreprocessorDirective(
        const std::string& line,
        std::size_t hashPosition)
    {
        for (std::size_t index = 0;
            index < hashPosition;
            ++index) {
            if (!std::isspace(
                static_cast<unsigned char>(line[index]))) {
                return false;
            }
        }

        std::size_t index = hashPosition + 1;

        while (index < line.size() &&
            std::isspace(
                static_cast<unsigned char>(line[index]))) {
            ++index;
        }

        const std::size_t wordStart = index;

        while (index < line.size() &&
            std::isalpha(
                static_cast<unsigned char>(line[index]))) {
            ++index;
        }

        const std::string directive =
            line.substr(wordStart, index - wordStart);

        static const std::set<std::string> directives = {
            "define",
            "elif",
            "else",
            "endif",
            "error",
            "if",
            "ifdef",
            "ifndef",
            "include",
            "line",
            "pragma",
            "undef",
            "warning"
        };

        return directives.count(directive) != 0;
    }

} // namespace

ParseResult AssemblyParser::parse(
    const std::filesystem::path& filePath) const
{
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Unable to open assembly source file: " +
            filePath.string()
        );
    }

    ParseResult result;
    result.filePath = filePath;

    const std::string architecture =
        detectArchitecture(filePath);

    AssemblyState state = AssemblyState::Normal;
    CommentBlock currentBlock;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        ++result.metrics.physicalLines;

        bool hasCode = false;
        bool hasComment = false;
        bool escaped = false;

        if (state == AssemblyState::Normal &&
            isWhitespaceOnly(line)) {
            ++result.metrics.blankLines;
            continue;
        }

        if (state == AssemblyState::BlockComment) {
            currentBlock.text += '\n';
        }

        std::size_t index = 0;

        while (index < line.size()) {
            const char current = line[index];

            const char next =
                index + 1 < line.size()
                ? line[index + 1]
                : '\0';

            if (state == AssemblyState::BlockComment) {
                hasComment = true;
                currentBlock.text += current;

                if (current == '*' && next == '/') {
                    currentBlock.text += '/';
                    currentBlock.endLine = lineNumber;

                    result.comments.push_back(
                        std::move(currentBlock)
                    );

                    currentBlock = CommentBlock{};
                    state = AssemblyState::Normal;
                    index += 2;
                }
                else {
                    ++index;
                }

                continue;
            }

            if (state == AssemblyState::StringLiteral) {
                hasCode = true;

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    state = AssemblyState::Normal;
                }

                ++index;
                continue;
            }

            if (state == AssemblyState::CharacterLiteral) {
                hasCode = true;

                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '\'') {
                    state = AssemblyState::Normal;
                }

                ++index;
                continue;
            }

            if (current == '/' && next == '*') {
                hasComment = true;

                currentBlock = CommentBlock{};
                currentBlock.type = CommentType::Block;
                currentBlock.startLine = lineNumber;
                currentBlock.endLine = lineNumber;
                currentBlock.text = "/*";

                state = AssemblyState::BlockComment;
                index += 2;
                continue;
            }

            if (current == '/' && next == '/') {
                hasComment = true;

                result.comments.push_back({
                    CommentType::Line,
                    lineNumber,
                    lineNumber,
                    line.substr(index)
                    });

                break;
            }

            if (current == '#' &&
                supportsHashComment(architecture)) {
                if (isPreprocessorDirective(line, index)) {
                    hasCode = true;
                    ++index;
                    continue;
                }

                hasComment = true;

                result.comments.push_back({
                    CommentType::Line,
                    lineNumber,
                    lineNumber,
                    line.substr(index)
                    });

                break;
            }

            if (current == '@' &&
                supportsAtComment(architecture)) {
                hasComment = true;

                result.comments.push_back({
                    CommentType::Line,
                    lineNumber,
                    lineNumber,
                    line.substr(index)
                    });

                break;
            }

            if (current == '"') {
                hasCode = true;
                escaped = false;
                state = AssemblyState::StringLiteral;
                ++index;
                continue;
            }

            if (current == '\'') {
                hasCode = true;
                escaped = false;
                state = AssemblyState::CharacterLiteral;
                ++index;
                continue;
            }

            if (!std::isspace(
                static_cast<unsigned char>(current))) {
                hasCode = true;
            }

            ++index;
        }

        if (state == AssemblyState::StringLiteral ||
            state == AssemblyState::CharacterLiteral) {
            state = AssemblyState::Normal;
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

    if (state == AssemblyState::BlockComment) {
        currentBlock.endLine = lineNumber;
        result.comments.push_back(
            std::move(currentBlock)
        );
    }

    return result;
}