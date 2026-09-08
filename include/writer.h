#pragma once

#include "parser.h"
#include "scanner.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

class JsonlWriter {
public:
    explicit JsonlWriter(
        const std::filesystem::path& outputPath
    );

    ~JsonlWriter();

    JsonlWriter(const JsonlWriter&) = delete;
    JsonlWriter& operator=(const JsonlWriter&) = delete;

    void write(
        const ParseResult& result,
        SourceLanguage language
    );

    std::size_t recordsWritten() const;

private:
    static constexpr std::size_t BufferLimit =
        4 * 1024 * 1024;

    void flushBuffer();

    std::ofstream output_;
    std::string buffer_;
    std::size_t recordsWritten_ = 0;
};