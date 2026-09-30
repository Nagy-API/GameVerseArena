#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstddef>
#include <sstream>
#include <string>

namespace text_layout_detail {
inline float widthOf(const sf::Font& font, const std::string& utf8, unsigned int size)
{
    const sf::Text probe(font, sf::String::fromUtf8(utf8.begin(), utf8.end()), size);
    return probe.getLocalBounds().size.x;
}

// Removes the last whole UTF-8 character (never a partial byte sequence).
inline void popLastCharacter(std::string& utf8)
{
    if (utf8.empty()) return;
    std::size_t index = utf8.size() - 1;
    while (index > 0 && (static_cast<unsigned char>(utf8[index]) & 0xC0) == 0x80) --index;
    utf8.erase(index);
}

// Removes the first whole UTF-8 character.
inline void popFirstCharacter(std::string& utf8)
{
    if (utf8.empty()) return;
    std::size_t length = 1;
    while (length < utf8.size() && (static_cast<unsigned char>(utf8[length]) & 0xC0) == 0x80) ++length;
    utf8.erase(0, length);
}
} // namespace text_layout_detail

// Word-wraps UTF-8 `text` so that no line is wider than `maxWidth` pixels at `size`, measured
// with the real font. At most `maxLines` lines are produced; overflow ends with "...".
inline std::string wrapToWidth(const sf::Font& font, const std::string& text, unsigned int size, float maxWidth,
                               std::size_t maxLines)
{
    using text_layout_detail::widthOf;
    std::istringstream words(text);
    std::string word;
    std::string line;
    std::string result;
    std::size_t lines = 0;
    bool truncated = false;
    while (words >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && widthOf(font, candidate, size) > maxWidth) {
            if (lines + 1 >= maxLines) {
                truncated = true;
                break;
            }
            result += line + "\n";
            ++lines;
            line = word;
        } else {
            line = candidate;
        }
    }
    if (truncated) {
        while (!line.empty() && widthOf(font, line + "...", size) > maxWidth) text_layout_detail::popLastCharacter(line);
        line += "...";
    }
    return result + line;
}

// Shortens a single UTF-8 line, keeping its beginning, so it fits `maxWidth` pixels.
inline std::string fitToWidth(const sf::Font& font, const std::string& text, unsigned int size, float maxWidth)
{
    using text_layout_detail::widthOf;
    if (widthOf(font, text, size) <= maxWidth) return text;
    std::string shortened = text;
    while (!shortened.empty() && widthOf(font, shortened + "...", size) > maxWidth) {
        text_layout_detail::popLastCharacter(shortened);
    }
    return shortened + "...";
}

// Shortens a single UTF-8 line, keeping its end (for text being typed), so it fits `maxWidth`.
inline std::string fitTailToWidth(const sf::Font& font, const std::string& text, unsigned int size, float maxWidth)
{
    using text_layout_detail::widthOf;
    if (widthOf(font, text, size) <= maxWidth) return text;
    std::string shortened = text;
    while (!shortened.empty() && widthOf(font, "..." + shortened, size) > maxWidth) {
        text_layout_detail::popFirstCharacter(shortened);
    }
    return "..." + shortened;
}
