#pragma once

#include "core/Types.h"

#include <filesystem>

namespace wot::storage {

struct AppSettings {
    Region region = Region::Lesta;
    bool english = false;
    bool promoFreeXp = false;
    bool premiumDiscount = false;
};

class SettingsStore {
public:
    SettingsStore();

    [[nodiscard]] AppSettings load() const;
    void save(const AppSettings& settings) const;
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace wot::storage
