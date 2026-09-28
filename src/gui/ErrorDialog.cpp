#include "ErrorDialog.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
std::wstring widen(const std::string& text)
{
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return std::wstring(text.begin(), text.end());
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    return wide;
}
} // namespace

void showErrorDialog(const std::string& utf8Title, const std::string& utf8Message)
{
    MessageBoxW(nullptr, widen(utf8Message).c_str(), widen(utf8Title).c_str(), MB_OK | MB_ICONERROR);
}
#else
void showErrorDialog(const std::string&, const std::string&)
{
}
#endif
