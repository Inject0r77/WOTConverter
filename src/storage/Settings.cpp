#include "storage/Settings.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#include <fstream>
#include <string>

namespace wot::storage {
namespace {

std::filesystem::path settingsPath() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        return std::filesystem::path(buffer) / L"Inject0r" / L"WoTConverter" / L"settings.ini";
    }
#endif
    return std::filesystem::current_path() / "settings.ini";
}

bool parseBool(const std::string& value, bool fallback) {
    if (value == "1" || value == "true" || value == "TRUE") return true;
    if (value == "0" || value == "false" || value == "FALSE") return false;
    return fallback;
}

} // namespace

SettingsStore::SettingsStore()
    : path_(settingsPath()) {}

AppSettings SettingsStore::load() const {
    AppSettings out;
    std::ifstream input(path_);
    if (!input) return out;

    std::string line;
    while (std::getline(input, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = line.substr(0, eq);
        const auto value = line.substr(eq + 1);
        if (key == "region") out.region = value == "WG" ? Region::WG : Region::Lesta;
        else if (key == "english") out.english = parseBool(value, out.english);
        else if (key == "promo_free_xp") out.promoFreeXp = parseBool(value, out.promoFreeXp);
        else if (key == "premium_discount") out.premiumDiscount = parseBool(value, out.premiumDiscount);
    }
    return out;
}

void SettingsStore::save(const AppSettings& settings) const {
    std::error_code ec;
    std::filesystem::create_directories(path_.parent_path(), ec);

    std::ofstream output(path_, std::ios::trunc);
    if (!output) return;
    output << "region=" << (settings.region == Region::WG ? "WG" : "LESTA") << '\n';
    output << "english=" << (settings.english ? 1 : 0) << '\n';
    output << "promo_free_xp=" << (settings.promoFreeXp ? 1 : 0) << '\n';
    output << "premium_discount=" << (settings.premiumDiscount ? 1 : 0) << '\n';
}

} // namespace wot::storage
