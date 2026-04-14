#include "Utf8.h"

#include <array>

namespace textsys::utf8
{
namespace
{

[[nodiscard]] bool isContinuationByte(unsigned char byte)
{
    return (byte & 0xC0u) == 0x80u;
}

[[nodiscard]] bool isInvalidCodepoint(char32_t codepoint)
{
    return codepoint > 0x10FFFFu || (codepoint >= 0xD800u && codepoint <= 0xDFFFu);
}

void appendReplacement(std::u32string& output, bool* hadErrors)
{
    output.push_back(kReplacementCharacter);
    if (hadErrors != nullptr)
    {
        *hadErrors = true;
    }
}

} // namespace

std::u32string decode(std::string_view utf8, bool* hadErrors)
{
    if (hadErrors != nullptr)
    {
        *hadErrors = false;
    }

    std::u32string output;
    output.reserve(utf8.size());

    std::size_t index = 0;
    while (index < utf8.size())
    {
        const unsigned char lead = static_cast<unsigned char>(utf8[index]);

        if (lead <= 0x7Fu)
        {
            output.push_back(static_cast<char32_t>(lead));
            ++index;
            continue;
        }

        std::size_t expectedCount = 0;
        char32_t codepoint = 0;
        char32_t minimumValue = 0;

        if ((lead & 0xE0u) == 0xC0u)
        {
            expectedCount = 2;
            codepoint = static_cast<char32_t>(lead & 0x1Fu);
            minimumValue = 0x80u;
        }
        else if ((lead & 0xF0u) == 0xE0u)
        {
            expectedCount = 3;
            codepoint = static_cast<char32_t>(lead & 0x0Fu);
            minimumValue = 0x800u;
        }
        else if ((lead & 0xF8u) == 0xF0u)
        {
            expectedCount = 4;
            codepoint = static_cast<char32_t>(lead & 0x07u);
            minimumValue = 0x10000u;
        }
        else
        {
            appendReplacement(output, hadErrors);
            ++index;
            continue;
        }

        if (index + expectedCount > utf8.size())
        {
            appendReplacement(output, hadErrors);
            break;
        }

        bool valid = true;
        for (std::size_t offset = 1; offset < expectedCount; ++offset)
        {
            const unsigned char continuation = static_cast<unsigned char>(utf8[index + offset]);
            if (!isContinuationByte(continuation))
            {
                valid = false;
                break;
            }

            codepoint = (codepoint << 6u) | static_cast<char32_t>(continuation & 0x3Fu);
        }

        if (!valid || codepoint < minimumValue || isInvalidCodepoint(codepoint))
        {
            appendReplacement(output, hadErrors);
            ++index;
            continue;
        }

        output.push_back(codepoint);
        index += expectedCount;
    }

    return output;
}

std::u32string decode(std::u8string_view utf8, bool* hadErrors)
{
    const auto* rawData = reinterpret_cast<const char*>(utf8.data());
    return decode(std::string_view(rawData, utf8.size()), hadErrors);
}

std::string encode(std::u32string_view codepoints, bool* hadErrors)
{
    if (hadErrors != nullptr)
    {
        *hadErrors = false;
    }

    std::string output;
    output.reserve(codepoints.size() * 4u);

    const auto appendCodepoint = [&output](char32_t codepoint)
    {
        if (codepoint <= 0x7Fu)
        {
            output.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FFu)
        {
            output.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        }
        else if (codepoint <= 0xFFFFu)
        {
            output.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        }
        else
        {
            output.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
            output.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
            output.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        }
    };

    for (char32_t codepoint : codepoints)
    {
        if (isInvalidCodepoint(codepoint))
        {
            if (hadErrors != nullptr)
            {
                *hadErrors = true;
            }
            appendCodepoint(kReplacementCharacter);
            continue;
        }

        appendCodepoint(codepoint);
    }

    return output;
}

bool isValid(std::string_view utf8)
{
    bool hadErrors = false;
    static_cast<void>(decode(utf8, &hadErrors));
    return !hadErrors;
}

bool isValid(std::u8string_view utf8)
{
    bool hadErrors = false;
    static_cast<void>(decode(utf8, &hadErrors));
    return !hadErrors;
}

} // namespace textsys::utf8
