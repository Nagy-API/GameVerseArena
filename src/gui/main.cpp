#include "Application.hpp"

#include <filesystem>

int main(int argc, char* argv[])
{
    std::filesystem::path executableDirectory = std::filesystem::current_path();
    if (argc > 0 && argv[0] != nullptr) {
        const auto executablePath = std::filesystem::absolute(argv[0]);
        if (executablePath.has_parent_path()) {
            executableDirectory = executablePath.parent_path();
        }
    }

    Application application(std::move(executableDirectory));
    return application.run();
}
