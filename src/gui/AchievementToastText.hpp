#pragma once

#include <cstddef>
#include <sstream>
#include <string>

inline std::string wrapAchievementToastDescription(const std::string& text, std::size_t maximumCharacters)
{
    if (maximumCharacters == 0) return text;
    std::istringstream words(text);
    std::ostringstream wrapped;
    std::string word;
    std::size_t lineLength = 0;
    while (words >> word) {
        if (lineLength != 0 && lineLength + 1 + word.size() > maximumCharacters) {
            wrapped << '\n';
            lineLength = 0;
        } else if (lineLength != 0) {
            wrapped << ' ';
            ++lineLength;
        }
        wrapped << word;
        lineLength += word.size();
    }
    return wrapped.str();
}
