#include "DatabasePaths.hpp"

#include <cstdlib>

namespace persistence {
namespace {
std::filesystem::path environmentPath(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0' ? std::filesystem::path(value) : std::filesystem::path{};
}
} // namespace

std::filesystem::path DatabasePaths::productionDatabasePath()
{
    std::filesystem::path base;
#ifdef _WIN32
    base = environmentPath("LOCALAPPDATA");
#else
    base = environmentPath("XDG_DATA_HOME");
    if (base.empty()) {
        const auto home = environmentPath("HOME");
        if (!home.empty()) {
            base = home / ".local" / "share";
        }
    }
#endif
    if (base.empty()) {
        base = std::filesystem::temp_directory_path();
    }

    return base / "GameVerseArena" / "gameverse.db";
}

} // namespace persistence
