#ifdef _WIN32

#include "core/Rates.h"
#include "storage/Settings.h"
#include "ui/MainWindow.h"
#include "ui/WindowsHeaders.h"

#include <commctrl.h>

#include <filesystem>
#include <string>
#include <vector>
#include <utility>

namespace {

std::filesystem::path executableDirectory() {
    std::vector<wchar_t> buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return std::filesystem::current_path();
    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok) {
        MessageBoxW(nullptr, L"Unable to initialize GDI+.", L"WoT Converter", MB_OK | MB_ICONERROR);
        return 1;
    }

    const auto exeDir = executableDirectory();
    const auto ratesPath = exeDir / L"config" / L"rates.json";
    std::string warning;
    auto rates = wot::RatesRepository::loadOrDefaults(ratesPath, &warning);

    wot::storage::SettingsStore settings;
    wot::ui::MainWindow window(instance, std::move(rates), std::move(settings), ratesPath, warning);

    int exitCode = 1;
    if (window.create()) {
        exitCode = window.run();
    } else {
        MessageBoxW(nullptr, L"Unable to create the main window.", L"WoT Converter", MB_OK | MB_ICONERROR);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return exitCode;
}

#endif // _WIN32
