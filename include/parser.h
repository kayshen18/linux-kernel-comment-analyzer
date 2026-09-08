#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

enum class CommentType {
    Line,
    Block
};

struct CommentBlock {
    CommentType type = CommentType::Line;
    std::size_t startLine = 0;
    std::size_t endLine = 0;
    std::string text;
};

struct FileMetrics {
    std::size_t physicalLines = 0;
    std::size_t codeLines = 0;
    std::size_t commentLines = 0;
    std::size_t mixedLines = 0;
    std::size_t blankLines = 0;
};

struct ParseResult {
    std::filesystem::path filePath;
    FileMetrics metrics;
    std::vector<CommentBlock> comments;
};

class CFamilyParser {
public:
    ParseResult parse(
        const std::filesystem::path& filePath
    ) const;
};

class PythonParser {
public:
    ParseResult parse(
        const std::filesystem::path& filePath
    ) const;
};

class ShellParser {
public:
    ParseResult parse(
        const std::filesystem::path& filePath
    ) const;
};

class MakefileParser {
public:
    ParseResult parse(
        const std::filesystem::path& filePath
    ) const;
};

class AssemblyParser {
public:
    ParseResult parse(
        const std::filesystem::path& filePath
    ) const;
};