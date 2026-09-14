#include "core/Converter.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace wot {

Converter::Converter(RatesConfig rates)
    : rates_(std::move(rates)) {}

double Converter::lestaRubFromGold(double gold) const {
    if (!std::isfinite(gold) || gold <= 0.0) return 0.0;
    const auto& p = rates_.lesta.goldPricing;
    if (gold <= p.tier1MaxGold) {
        return gold * p.tier1RubPerGold;
    }
    if (gold <= p.tier2MaxGold) {
        return p.tier2BaseRub + (gold - p.tier1MaxGold) * p.tier2RubPerGold;
    }
    return gold * p.tier3RubPerGold;
}

double Converter::lestaGoldFromRub(double rub) const {
    if (!std::isfinite(rub) || rub <= 0.0) return 0.0;
    const auto& p = rates_.lesta.goldPricing;
    const double tier1RubMax = p.tier1MaxGold * p.tier1RubPerGold;
    const double tier2RubMax = p.tier2BaseRub + (p.tier2MaxGold - p.tier1MaxGold) * p.tier2RubPerGold;

    if (rub <= tier1RubMax) {
        return rub / p.tier1RubPerGold;
    }
    if (rub <= tier2RubMax) {
        return p.tier1MaxGold + (rub - p.tier2BaseRub) / p.tier2RubPerGold;
    }
    return rub / p.tier3RubPerGold;
}

ConversionResult Converter::convert(const ConversionInput& input) const {
    if (!std::isfinite(input.value) || input.value < 0.0) {
        throw std::invalid_argument("conversion input must be a finite non-negative number");
    }

    const CommonRates& common = input.region == Region::Lesta ? rates_.lesta.common : rates_.wg.common;
    const double xpRate = input.modifiers.promoFreeXp ? common.promoFreeXpPerGold : common.freeXpPerGold;

    double gold = 0.0;
    switch (input.source) {
        case SourceKind::Money:
            gold = input.region == Region::Lesta
                ? lestaGoldFromRub(input.value)
                : input.value / rates_.wg.eurPerGold;
            break;
        case SourceKind::Gold:
            gold = input.value;
            break;
        case SourceKind::Credits:
            gold = input.value / common.creditsPerGold;
            break;
        case SourceKind::FreeXp:
            gold = input.value / xpRate;
            break;
    }

    ConversionResult out;
    out.gold = std::max(0.0, gold);
    out.money = input.region == Region::Lesta
        ? lestaRubFromGold(out.gold)
        : out.gold * rates_.wg.eurPerGold;
    out.credits = out.gold * common.creditsPerGold;
    out.freeXp = out.gold * xpRate;

    const double premiumCost = common.premiumGold30Days * (input.modifiers.premiumShopDiscount ? 0.85 : 1.0);
    out.premiumDays = premiumCost > 0.0 ? (out.gold / premiumCost) * 30.0 : 0.0;
    return out;
}

} // namespace wot
