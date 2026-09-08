#include "scanner.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <system_error>

namespace {

    std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );

        return value;
    }

} // namespace

SourceScanner::SourceScanner(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory))
{
}

std::vector<SourceFile> SourceScanner::scan() const
{
    std::vector<SourceFile> sourceFiles;

    if (!std::filesystem::exists(rootDirectory_)) {
        std::cerr
            << "Error: source directory does not exist: "
            << rootDirectory_
            << '\n';

        return sourceFiles;
    }

    if (!std::filesystem::is_directory(rootDirectory_)) {
        std::cerr
            << "Error: source path is not a directory: "
            << rootDirectory_
            << '\n';

        return sourceFiles;
    }

    std::error_code error;

    const auto options =
        std::filesystem::directory_options::skip_permission_denied;

    std::filesystem::recursive_directory_iterator iterator(
        rootDirectory_,
        options,
        error
    );

    const std::filesystem::recursive_directory_iterator end;

    while (iterator != end) {
        if (error) {
            std::cerr
                << "Warning: unable to access an entry: "
                << error.message()
                << '\n';

            error.clear();
            iterator.increment(error);
            continue;
        }

        const auto& entry = *iterator;

        // 不跟随目录符号链接，避免循环遍历。
        if (entry.is_symlink(error)) {
            if (entry.is_directory(error)) {
                iterator.disable_recursion_pending();
            }

            error.clear();
            iterator.increment(error);
            continue;
        }

        if (entry.is_regular_file(error)) {
            const SourceLanguage language =
                detectLanguage(entry.path());

            if (language != SourceLanguage::Unknown) {
                std::uintmax_t fileSize = entry.file_size(error);

                if (error) {
                    fileSize = 0;
                    error.clear();
                }

                sourceFiles.push_back({
                    entry.path(),
                    language,
                    fileSize
                    });
            }
        }

        error.clear();
        iterator.increment(error);
    }

    return sourceFiles;
}

SourceLanguage SourceScanner::detectLanguage(
    const std::filesystem::path& filePath)
{
    const std::string fileName = filePath.filename().string();
    const std::string originalExtension =
        filePath.extension().string();
    const std::string extension =
        toLower(originalExtension);

    if (fileName == "Makefile") {
        return SourceLanguage::Makefile;
    }

    if (extension == ".c" || extension == ".h") {
        return SourceLanguage::C;
    }

    if (extension == ".cc" ||
        extension == ".cpp" ||
        extension == ".cxx" ||
        extension == ".hpp") {
        return SourceLanguage::Cpp;
    }

    // 同时支持 .s 和经过预处理的 .S 汇编文件。
    if (extension == ".s") {
        return SourceLanguage::Assembly;
    }

    if (extension == ".py") {
        return SourceLanguage::Python;
    }

    if (extension == ".sh") {
        return SourceLanguage::Shell;
    }

    return SourceLanguage::Unknown;
}

std::string toString(SourceLanguage language)
{
    switch (language) {
    case SourceLanguage::C:
        return "C";

    case SourceLanguage::Cpp:
        return "C++";

    case SourceLanguage::Assembly:
        return "Assembly";

    case SourceLanguage::Python:
        return "Python";

    case SourceLanguage::Shell:
        return "Shell";

    case SourceLanguage::Makefile:
        return "Makefile";

    default:
        return "Unknown";
    }
}