#include "DatabasePaths.hpp"

#include <algorithm>
#include <cstdlib>
#include <cwctype>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace persistence {
namespace {
std::filesystem::path environmentPath(const char* name)
{
#ifdef _WIN32
    // Read the variable as UTF-16 so profile folders with non-ASCII names (for example
    // C:\Users\José) resolve correctly instead of failing a narrow code-page conversion.
    std::wstring wideName(name, name + std::char_traits<char>::length(name));
    const DWORD required = GetEnvironmentVariableW(wideName.c_str(), nullptr, 0);
    if (required <= 1) return {};
    std::wstring value(required, L'\0');
    const DWORD written = GetEnvironmentVariableW(wideName.c_str(), value.data(), required);
    if (written == 0 || written >= required) return {};
    value.resize(written);
    return std::filesystem::path(value);
#else
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0' ? std::filesystem::path(value) : std::filesystem::path{};
#endif
}

std::filesystem::path normalised(const std::filesystem::path& value)
{
    std::error_code error;
    auto absolute = std::filesystem::absolute(value, error);
    if (error) absolute = value;
    auto canonical = std::filesystem::weakly_canonical(absolute, error);
    return (error ? absolute : canonical).lexically_normal();
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
        std::error_code error;
        base = std::filesystem::temp_directory_path(error);
        if (error) base = std::filesystem::current_path();
    }

    return base / "GameVerseArena" / "gameverse.db";
}

bool DatabasePaths::referToSameFile(const std::filesystem::path& first, const std::filesystem::path& second)
{
    std::error_code error;
    if (std::filesystem::exists(first, error) && std::filesystem::exists(second, error)) {
        const bool same = std::filesystem::equivalent(first, second, error);
        if (!error) return same;
    }
    auto left = normalised(first).native();
    auto right = normalised(second).native();
#ifdef _WIN32
    const auto lower = [](auto& text) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    };
    lower(left);
    lower(right);
#endif
    return left == right;
}

namespace {
// Windows ignores trailing dots and spaces in path components and compares names without
// regard to case; apply the same rules when comparing one component by name.
std::filesystem::path::string_type comparableName(std::filesystem::path::string_type name)
{
    while (!name.empty() && (name.back() == '.' || name.back() == ' ')) name.pop_back();
#ifdef _WIN32
    std::transform(name.begin(), name.end(), name.begin(),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
#endif
    return name;
}

bool sameExistingDirectory(const std::filesystem::path& first, const std::filesystem::path& second)
{
    std::error_code error;
    if (!std::filesystem::exists(first, error) || !std::filesystem::exists(second, error)) return false;
    const bool same = std::filesystem::equivalent(first, second, error);
    return !error && same;
}
} // namespace

bool DatabasePaths::insideProductionFolder(const std::filesystem::path& candidate)
{
    const auto production = productionDatabasePath();
    if (referToSameFile(candidate, production)) return true;
    const auto productionFolder = production.parent_path();  // ...\GameVerseArena
    const auto appDataFolder = productionFolder.parent_path();  // %LOCALAPPDATA% or equivalent

    std::error_code error;
    auto current = std::filesystem::absolute(candidate, error);
    if (error) current = candidate;
    std::filesystem::path child;
    while (!current.empty()) {
        if (sameExistingDirectory(current, productionFolder)) return true;
        if (!child.empty() && sameExistingDirectory(current, appDataFolder) &&
            comparableName(child.filename().native()) == comparableName(productionFolder.filename().native())) {
            return true;
        }
        const auto parent = current.parent_path();
        if (parent == current) break;
        child = current;
        current = parent;
    }
    return false;
}

} // namespace persistence
