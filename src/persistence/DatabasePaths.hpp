#pragma once

#include <filesystem>

namespace persistence {

class DatabasePaths {
public:
    static std::filesystem::path productionDatabasePath();

    // True when both paths name the same file, even when spelled differently (letter case on
    // Windows, "." / ".." segments, or links to an existing file). Developer tools use it to
    // refuse the production database.
    static bool referToSameFile(const std::filesystem::path& first, const std::filesystem::path& second);

    // True when `candidate` is the production database or lies inside its folder, including
    // spellings that only resolve through the file system (8.3 names, "\\?\" prefixes, trailing
    // dots or spaces), and even before the production folder exists. Used to keep developer
    // tools away from real player data.
    static bool insideProductionFolder(const std::filesystem::path& candidate);
};

} // namespace persistence
