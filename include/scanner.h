#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

enum class SourceLanguage {
    C,
    Cpp,
    Assembly,
    Python,
    Shell,
    Makefile,
    Unknown
};

struct SourceFile {
    std::filesystem::path path;
    SourceLanguage language = SourceLanguage::Unknown;
    std::uintmax_t sizeBytes = 0;
};

class SourceScanner {
public:
    explicit SourceScanner(std::filesystem::path rootDirectory);

    std::vector<SourceFile> scan() const;

private:
    std::filesystem::path rootDirectory_;

    static SourceLanguage detectLanguage(
        const std::filesystem::path& filePath
    );
};

std::string toString(SourceLanguage language);