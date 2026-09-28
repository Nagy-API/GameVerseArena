#include "Application.hpp"
#include "DatabasePaths.hpp"
#include "ErrorDialog.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
void reportStartupError(const std::string& message, bool showDialog)
{
    std::cerr << "GameVerseArenaGUI: " << message << '\n';
    if (showDialog) showErrorDialog("GameVerseArenaGUI", message);
}

std::string usage()
{
    return "Usage: GameVerseArenaGUI [--database <path>] [--silent-audio] [--smoke-test <output-dir>]";
}

// Command-line arguments as paths. On Windows they are read as UTF-16 so non-ASCII paths
// survive; the narrow argv there is in the ANSI code page and cannot represent them.
std::vector<std::filesystem::path> commandLineArguments(int argc, char* argv[])
{
    std::vector<std::filesystem::path> arguments;
#ifdef _WIN32
    int count = 0;
    if (LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count)) {
        for (int index = 1; index < count; ++index) arguments.emplace_back(std::wstring(wide[index]));
        LocalFree(wide);
        return arguments;
    }
#endif
    for (int index = 1; index < argc; ++index) {
        if (argv[index] != nullptr) arguments.emplace_back(std::string(argv[index]));
    }
    return arguments;
}

std::filesystem::path executableDirectory(int argc, char* argv[])
{
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    for (int attempt = 0; attempt < 6; ++attempt) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) break;
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
    std::error_code error;
    if (argc > 0 && argv[0] != nullptr) {
        const auto executable = std::filesystem::absolute(std::filesystem::path(std::string(argv[0])), error);
        if (!error && executable.has_parent_path()) return executable.parent_path();
    }
    return std::filesystem::current_path();
}

bool isOption(const std::filesystem::path& argument, const char* option)
{
    return argument.native() == std::filesystem::path(option).native();
}
} // namespace

int main(int argc, char* argv[])
{
    bool showDialogs = true;
    try {
        const auto arguments = commandLineArguments(argc, argv);
        for (const auto& argument : arguments) {
            if (isOption(argument, "--smoke-test")) showDialogs = false;
        }

        // Unknown options are rejected rather than ignored so that a mistyped --database can
        // never fall back to the real profile database during testing.
        ApplicationOptions options;
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            const auto& argument = arguments[index];
            if (isOption(argument, "--database")) {
                if (index + 1 >= arguments.size() || arguments[index + 1].empty()) {
                    reportStartupError("--database requires a file path.\n\n" + usage(), showDialogs);
                    return 2;
                }
                options.databasePath = arguments[++index];
            } else if (isOption(argument, "--silent-audio")) {
                options.silentAudio = true;
            } else if (isOption(argument, "--smoke-test")) {
                if (index + 1 >= arguments.size() || arguments[index + 1].empty()) {
                    reportStartupError("--smoke-test requires an output directory.\n\n" + usage(), showDialogs);
                    return 2;
                }
                options.smokeTestDirectory = arguments[++index];
                options.silentAudio = true;
            } else {
                reportStartupError("Unknown option: " + argument.u8string() + "\n\n" + usage(), showDialogs);
                return 2;
            }
        }

        if (options.smokeTestDirectory) {
            // The smoke test creates profiles, history, and settings; it must only ever run
            // against a brand-new throwaway database, never the player's real one.
            std::error_code error;
            if (!options.databasePath) {
                reportStartupError("--smoke-test also requires --database <new-file>.\n\n" + usage(), false);
                return 2;
            }
            if (std::filesystem::exists(*options.databasePath, error) || error) {
                reportStartupError("--smoke-test requires a --database path that does not exist yet.", false);
                return 2;
            }
            if (persistence::DatabasePaths::insideProductionFolder(*options.databasePath)) {
                reportStartupError("--smoke-test refuses to use the production profile database or its folder.", false);
                return 2;
            }
        }

        Application application(executableDirectory(argc, argv), std::move(options));
        return application.run();
    } catch (const std::exception& error) {
        reportStartupError(std::string("GameVerseArena stopped because of an unexpected error:\n\n") + error.what(),
                           showDialogs);
    } catch (...) {
        reportStartupError("GameVerseArena stopped because of an unexpected error.", showDialogs);
    }
    return 1;
}
