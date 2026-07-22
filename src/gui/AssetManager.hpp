#pragma once

#include <SFML/Graphics/Font.hpp>

#include <filesystem>
#include <string>

class AssetManager {
public:
    bool load(const std::filesystem::path& assetRoot, std::string& error)
    {
        const auto fontRoot = assetRoot / "fonts";
        const auto regularPath = fontRoot / "Inter-Regular.ttf";
        const auto semiboldPath = fontRoot / "Inter-SemiBold.ttf";

        if (!regularFont_.openFromFile(regularPath)) {
            error = "Could not load required font: " + regularPath.string();
            return false;
        }
        if (!semiboldFont_.openFromFile(semiboldPath)) {
            error = "Could not load required font: " + semiboldPath.string();
            return false;
        }
        return true;
    }

    const sf::Font& regularFont() const noexcept { return regularFont_; }
    const sf::Font& semiboldFont() const noexcept { return semiboldFont_; }

private:
    sf::Font regularFont_;
    sf::Font semiboldFont_;
};
