#include "core/Rates.h"

#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace wot {
namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("rates config not found");
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string extractObject(const std::string& text, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto keyPos = text.find(needle);
    if (keyPos == std::string::npos) return {};
    const auto braceStart = text.find('{', keyPos + needle.size());
    if (braceStart == std::string::npos) return {};

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = braceStart; i < text.size(); ++i) {
        const char ch = text[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') inString = false;
            continue;
        }
        if (ch == '"') {
            inString = true;
            continue;
        }
        if (ch == '{') ++depth;
        else if (ch == '}') {
            --depth;
            if (depth == 0) return text.substr(braceStart, i - braceStart + 1);
        }
    }
    return {};
}

std::string extractArray(const std::string& text, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto keyPos = text.find(needle);
    if (keyPos == std::string::npos) return {};
    const auto arrayStart = text.find('[', keyPos + needle.size());
    if (arrayStart == std::string::npos) return {};

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = arrayStart; i < text.size(); ++i) {
        const char ch = text[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') inString = false;
            continue;
        }
        if (ch == '"') {
            inString = true;
            continue;
        }
        if (ch == '[') ++depth;
        else if (ch == ']') {
            --depth;
            if (depth == 0) return text.substr(arrayStart, i - arrayStart + 1);
        }
    }
    return {};
}

double numberValue(const std::string& text, const std::string& key, double fallback) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) return fallback;
    try {
        return std::stod(match[1].str());
    } catch (...) {
        return fallback;
    }
}

int intValue(const std::string& text, const std::string& key, int fallback) {
    return static_cast<int>(std::llround(numberValue(text, key, static_cast<double>(fallback))));
}

std::vector<BoxPackage> parsePackages(const std::string& text, const std::vector<BoxPackage>& fallback) {
    const auto array = extractArray(text, "box_packages");
    if (array.empty()) return fallback;

    std::vector<BoxPackage> result;
    std::size_t pos = 0;
    while (true) {
        const auto begin = array.find('{', pos);
        if (begin == std::string::npos) break;
        const auto end = array.find('}', begin + 1);
        if (end == std::string::npos) break;
        const auto object = array.substr(begin, end - begin + 1);
        const int count = intValue(object, "count", 0);
        const double price = numberValue(object, "price", 0.0);
        if (count > 0 && price > 0.0) result.push_back({count, price});
        pos = end + 1;
    }
    return result.empty() ? fallback : result;
}

void validate(RatesConfig& rates, const RatesConfig& defaults) {
    auto positive = [](double value, double fallback) {
        return std::isfinite(value) && value > 0.0 ? value : fallback;
    };

    rates.lesta.common.creditsPerGold = positive(rates.lesta.common.creditsPerGold, defaults.lesta.common.creditsPerGold);
    rates.lesta.common.freeXpPerGold = positive(rates.lesta.common.freeXpPerGold, defaults.lesta.common.freeXpPerGold);
    rates.lesta.common.promoFreeXpPerGold = positive(rates.lesta.common.promoFreeXpPerGold, defaults.lesta.common.promoFreeXpPerGold);
    rates.lesta.common.premiumGold30Days = positive(rates.lesta.common.premiumGold30Days, defaults.lesta.common.premiumGold30Days);
    rates.lesta.singleBoxPriceRub = positive(rates.lesta.singleBoxPriceRub, defaults.lesta.singleBoxPriceRub);

    rates.lesta.goldPricing.tier1MaxGold = positive(rates.lesta.goldPricing.tier1MaxGold, defaults.lesta.goldPricing.tier1MaxGold);
    rates.lesta.goldPricing.tier1RubPerGold = positive(rates.lesta.goldPricing.tier1RubPerGold, defaults.lesta.goldPricing.tier1RubPerGold);
    rates.lesta.goldPricing.tier2MaxGold = positive(rates.lesta.goldPricing.tier2MaxGold, defaults.lesta.goldPricing.tier2MaxGold);
    rates.lesta.goldPricing.tier2BaseRub = positive(rates.lesta.goldPricing.tier2BaseRub, defaults.lesta.goldPricing.tier2BaseRub);
    rates.lesta.goldPricing.tier2RubPerGold = positive(rates.lesta.goldPricing.tier2RubPerGold, defaults.lesta.goldPricing.tier2RubPerGold);
    rates.lesta.goldPricing.tier3RubPerGold = positive(rates.lesta.goldPricing.tier3RubPerGold, defaults.lesta.goldPricing.tier3RubPerGold);

    rates.wg.common.creditsPerGold = positive(rates.wg.common.creditsPerGold, defaults.wg.common.creditsPerGold);
    rates.wg.common.freeXpPerGold = positive(rates.wg.common.freeXpPerGold, defaults.wg.common.freeXpPerGold);
    rates.wg.common.promoFreeXpPerGold = positive(rates.wg.common.promoFreeXpPerGold, defaults.wg.common.promoFreeXpPerGold);
    rates.wg.common.premiumGold30Days = positive(rates.wg.common.premiumGold30Days, defaults.wg.common.premiumGold30Days);
    rates.wg.eurPerGold = positive(rates.wg.eurPerGold, defaults.wg.eurPerGold);
    rates.wg.singleBoxFallbackPrice = positive(rates.wg.singleBoxFallbackPrice, defaults.wg.singleBoxFallbackPrice);
    if (rates.wg.boxPackages.empty()) rates.wg.boxPackages = defaults.wg.boxPackages;
}

} // namespace

RatesConfig RatesRepository::defaults() {
    RatesConfig config;
    config.wg.boxPackages = {
        {230, 264.86},
        {120, 146.78},
        {80, 103.65},
        {30, 40.15},
        {5, 6.83}
    };
    return config;
}

RatesConfig RatesRepository::loadOrDefaults(const std::filesystem::path& path, std::string* warning) {
    const auto fallback = defaults();
    try {
        const auto text = readFile(path);
        RatesConfig result = fallback;
        result.schemaVersion = intValue(text, "schema_version", fallback.schemaVersion);

        const auto lesta = extractObject(text, "lesta");
        const auto wg = extractObject(text, "wg");
        const auto lestaPricing = extractObject(lesta, "gold_pricing");

        if (!lesta.empty()) {
            result.lesta.common.creditsPerGold = numberValue(lesta, "credits_per_gold", result.lesta.common.creditsPerGold);
            result.lesta.common.freeXpPerGold = numberValue(lesta, "free_xp_per_gold", result.lesta.common.freeXpPerGold);
            result.lesta.common.promoFreeXpPerGold = numberValue(lesta, "promo_free_xp_per_gold", result.lesta.common.promoFreeXpPerGold);
            result.lesta.common.premiumGold30Days = numberValue(lesta, "premium_gold_30_days", result.lesta.common.premiumGold30Days);
            result.lesta.singleBoxPriceRub = numberValue(lesta, "single_box_price", result.lesta.singleBoxPriceRub);
        }
        if (!lestaPricing.empty()) {
            result.lesta.goldPricing.tier1MaxGold = numberValue(lestaPricing, "tier1_max_gold", result.lesta.goldPricing.tier1MaxGold);
            result.lesta.goldPricing.tier1RubPerGold = numberValue(lestaPricing, "tier1_rub_per_gold", result.lesta.goldPricing.tier1RubPerGold);
            result.lesta.goldPricing.tier2MaxGold = numberValue(lestaPricing, "tier2_max_gold", result.lesta.goldPricing.tier2MaxGold);
            result.lesta.goldPricing.tier2BaseRub = numberValue(lestaPricing, "tier2_base_rub", result.lesta.goldPricing.tier2BaseRub);
            result.lesta.goldPricing.tier2RubPerGold = numberValue(lestaPricing, "tier2_rub_per_gold", result.lesta.goldPricing.tier2RubPerGold);
            result.lesta.goldPricing.tier3RubPerGold = numberValue(lestaPricing, "tier3_rub_per_gold", result.lesta.goldPricing.tier3RubPerGold);
        }

        if (!wg.empty()) {
            result.wg.common.creditsPerGold = numberValue(wg, "credits_per_gold", result.wg.common.creditsPerGold);
            result.wg.common.freeXpPerGold = numberValue(wg, "free_xp_per_gold", result.wg.common.freeXpPerGold);
            result.wg.common.promoFreeXpPerGold = numberValue(wg, "promo_free_xp_per_gold", result.wg.common.promoFreeXpPerGold);
            result.wg.common.premiumGold30Days = numberValue(wg, "premium_gold_30_days", result.wg.common.premiumGold30Days);
            result.wg.eurPerGold = numberValue(wg, "eur_per_gold", result.wg.eurPerGold);
            result.wg.singleBoxFallbackPrice = numberValue(wg, "single_box_fallback_price", result.wg.singleBoxFallbackPrice);
            result.wg.boxPackages = parsePackages(wg, result.wg.boxPackages);
        }

        validate(result, fallback);
        if (warning) warning->clear();
        return result;
    } catch (const std::exception& ex) {
        if (warning) *warning = ex.what();
        return fallback;
    }
}

} // namespace wot
