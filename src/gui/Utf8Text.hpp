#pragma once

#include <cstddef>
#include <string>

namespace utf8_text {

inline std::size_t length(const std::string& value)
{
    std::size_t count = 0;
    for (unsigned char byte : value) if ((byte & 0xC0) != 0x80) ++count;
    return count;
}

inline bool appendPrintable(std::string& value, char32_t codepoint, std::size_t maximumLength)
{
    if (codepoint < 32 || codepoint > 0x10FFFF ||
        (codepoint >= 0x7F && codepoint <= 0x9F) || length(value) >= maximumLength) {
        return false;
    }
    if (codepoint <= 0x7F) {
        value.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        value.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        value.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        value.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return true;
}

inline void eraseLast(std::string& value)
{
    if (value.empty()) return;
    std::size_t index = value.size() - 1;
    while (index > 0 && (static_cast<unsigned char>(value[index]) & 0xC0) == 0x80) --index;
    value.erase(index);
}

inline std::string tail(const std::string& value, std::size_t maximumLength)
{
    const auto total = length(value);
    if (total <= maximumLength) return value;
    const std::size_t charactersToSkip = total - maximumLength;
    std::size_t skipped = 0;
    std::size_t index = 0;
    while (index < value.size() && skipped < charactersToSkip) {
        ++index;
        while (index < value.size() && (static_cast<unsigned char>(value[index]) & 0xC0) == 0x80) ++index;
        ++skipped;
    }
    return value.substr(index);
}

} // namespace utf8_text
