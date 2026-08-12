#pragma once

#include <filesystem>

namespace persistence {

class DatabasePaths {
public:
    static std::filesystem::path productionDatabasePath();
};

} // namespace persistence
