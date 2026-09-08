#include "writer.h"

#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void appendEscapedJson(
        std::string& destination,
        std::string_view value)
    {
        static constexpr char hexDigits[] =
            "0123456789abcdef";

        for (unsigned char character : value) {
            switch (character) {
            case '"':
                destination += "\\\"";
                break;

            case '\\':
                destination += "\\\\";
                break;

            case '\b':
                destination += "\\b";
                break;

            case '\f':
                destination += "\\f";
                break;

            case '\n':
                destination += "\\n";
                break;

            case '\r':
                destination += "\\r";
                break;

            case '\t':
                destination += "\\t";
                break;

            default:
                if (character < 0x20) {
                    destination += "\\u00";
                    destination +=
                        hexDigits[(character >> 4) & 0x0f];
                    destination +=
                        hexDigits[character & 0x0f];
                }
                else {
                    destination +=
                        static_cast<char>(character);
                }

                break;
            }
        }
    }

    std::string_view commentTypeToString(CommentType type)
    {
        switch (type) {
        case CommentType::Line:
            return "line";

        case CommentType::Block:
            return "block";

        default:
            return "unknown";
        }
    }

} // namespace

JsonlWriter::JsonlWriter(
    const std::filesystem::path& outputPath)
{
    const std::filesystem::path parent =
        outputPath.parent_path();

    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    output_.open(
        outputPath,
        std::ios::out |
        std::ios::binary |
        std::ios::trunc
    );

    if (!output_.is_open()) {
        throw std::runtime_error(
            "Unable to create output file: " +
            outputPath.string()
        );
    }

    buffer_.reserve(BufferLimit + 64 * 1024);
}

JsonlWriter::~JsonlWriter()
{
    /*
     * 析构函数不能向外抛异常，因此在此完成最后一批写入。
     * 正常批次中的错误由 flushBuffer() 报告。
     */
    if (output_.is_open() && !buffer_.empty()) {
        output_.write(
            buffer_.data(),
            static_cast<std::streamsize>(
                buffer_.size()
                )
        );

        buffer_.clear();
    }

    if (output_.is_open()) {
        output_.flush();
    }
}

void JsonlWriter::flushBuffer()
{
    if (buffer_.empty()) {
        return;
    }

    output_.write(
        buffer_.data(),
        static_cast<std::streamsize>(
            buffer_.size()
            )
    );

    if (!output_) {
        throw std::runtime_error(
            "Failed while writing buffered JSONL output"
        );
    }

    buffer_.clear();
}

void JsonlWriter::write(
    const ParseResult& result,
    SourceLanguage language)
{
    const std::string languageName =
        toString(language);

    for (const CommentBlock& comment : result.comments) {
        buffer_ += "{\"file\":\"";
        appendEscapedJson(
            buffer_,
            result.filePath.generic_string()
        );

        buffer_ += "\",\"language\":\"";
        appendEscapedJson(
            buffer_,
            languageName
        );

        buffer_ += "\",\"type\":\"";
        buffer_ += commentTypeToString(comment.type);

        buffer_ += "\",\"start_line\":";
        buffer_ += std::to_string(comment.startLine);

        buffer_ += ",\"end_line\":";
        buffer_ += std::to_string(comment.endLine);

        buffer_ += ",\"text\":\"";
        appendEscapedJson(
            buffer_,
            comment.text
        );

        buffer_ += "\"}\n";

        ++recordsWritten_;

        if (buffer_.size() >= BufferLimit) {
            flushBuffer();
        }
    }
}

std::size_t JsonlWriter::recordsWritten() const
{
    return recordsWritten_;
}