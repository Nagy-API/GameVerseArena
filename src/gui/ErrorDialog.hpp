#pragma once

#include <string>

// Shows a modal error message (Windows only; a no-op elsewhere). Both strings are UTF-8 and
// are converted to UTF-16, so paths and names with non-ASCII characters display correctly.
void showErrorDialog(const std::string& utf8Title, const std::string& utf8Message);
