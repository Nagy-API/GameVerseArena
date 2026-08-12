#include "Application.hpp"

#include <filesystem>
#include <optional>
#include <string>

int main(int argc, char* argv[])
{
    std::filesystem::path executableDirectory = std::filesystem::current_path();
    if (argc > 0 && argv[0] != nullptr) {
        const auto executablePath = std::filesystem::absolute(argv[0]);
        if (executablePath.has_parent_path()) {
            executableDirectory = executablePath.parent_path();
        }
    }

    std::optional<std::filesystem::path> databasePath;
    if (argc == 3 && argv[1] != nullptr && std::string(argv[1]) == "--database" && argv[2] != nullptr) {
        databasePath = std::filesystem::path(argv[2]);
    }

    Application application(std::move(executableDirectory), std::move(databasePath));
    return application.run();
}
