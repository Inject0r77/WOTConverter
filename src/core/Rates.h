#pragma once

#include "core/Types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace wot {

struct LestaGoldPricing {
    double tier1MaxGold = 100.0;
    double tier1RubPerGold = 0.16;
    double tier2MaxGold = 500.0;
    double tier2BaseRub = 16.0;
    double tier2RubPerGold = 0.155;
    double tier3RubPerGold = 0.156;
};

struct CommonRates {
    double creditsPerGold = 400.0;
    double freeXpPerGold = 25.0;
    double promoFreeXpPerGold = 35.0;
    double premiumGold30Days = 2500.0;
};

struct LestaRates {
    CommonRates common{};
    LestaGoldPricing goldPricing{};
    double singleBoxPriceRub = 39.0;
};

struct WgRates {
    CommonRates common{};
    double eurPerGold = 0.0036;
    std::vector<BoxPackage> boxPackages{};
    double singleBoxFallbackPrice = 6.83 / 5.0;
};

struct RatesConfig {
    int schemaVersion = 1;
    LestaRates lesta{};
    WgRates wg{};
};

class RatesRepository {
public:
    static RatesConfig defaults();
    static RatesConfig loadOrDefaults(const std::filesystem::path& path, std::string* warning = nullptr);
};

} // namespace wot
