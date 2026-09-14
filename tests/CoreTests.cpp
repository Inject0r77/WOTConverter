#include "core/BoxOptimizer.h"
#include "core/Converter.h"
#include "core/Rates.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void expectNear(double actual, double expected, double eps, const std::string& label) {
    if (std::abs(actual - expected) > eps) {
        std::cerr << "FAIL: " << label << " actual=" << actual << " expected=" << expected << '\n';
        std::exit(1);
    }
}

void expect(bool condition, const std::string& label) {
    if (!condition) {
        std::cerr << "FAIL: " << label << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    std::string configWarning;
    const auto loadedRates = wot::RatesRepository::loadOrDefaults(
        std::filesystem::path(WOT_SOURCE_DIR) / "config" / "rates.json", &configWarning);
    expect(configWarning.empty(), "rates.json loads without fallback");
    expectNear(loadedRates.lesta.singleBoxPriceRub, 39.0, 1e-9, "rates.json LESTA box price");
    expectNear(loadedRates.wg.eurPerGold, 0.0036, 1e-12, "rates.json WG EUR rate");
    expect(loadedRates.wg.boxPackages.size() == 5, "rates.json WG package count");

    const auto rates = wot::RatesRepository::defaults();
    wot::Converter converter(rates);

    expectNear(converter.lestaRubFromGold(100.0), 16.0, 1e-9, "LESTA 100 gold -> RUB");
    expectNear(converter.lestaRubFromGold(500.0), 78.0, 1e-9, "LESTA 500 gold -> RUB");
    expectNear(converter.lestaGoldFromRub(78.0), 500.0, 1e-9, "LESTA 78 RUB -> gold");

    wot::ConversionInput input;
    input.region = wot::Region::Lesta;
    input.source = wot::SourceKind::Gold;
    input.value = 2500.0;
    auto result = converter.convert(input);
    expectNear(result.credits, 1'000'000.0, 1e-6, "credits conversion");
    expectNear(result.freeXp, 62'500.0, 1e-6, "free XP conversion");
    expectNear(result.premiumDays, 30.0, 1e-9, "premium conversion");

    input.modifiers.promoFreeXp = true;
    input.modifiers.premiumShopDiscount = true;
    result = converter.convert(input);
    expectNear(result.freeXp, 87'500.0, 1e-6, "promo free XP conversion");
    expectNear(result.premiumDays, 2500.0 / 2125.0 * 30.0, 1e-9, "premium discount conversion");

    input.region = wot::Region::WG;
    input.source = wot::SourceKind::Money;
    input.value = 36.0;
    result = converter.convert(input);
    expectNear(result.gold, 10'000.0, 1e-6, "WG EUR -> gold");

    wot::BoxOptimizer boxes(rates);
    const auto lestaBoxes = boxes.calculate(wot::Region::Lesta, 30);
    expectNear(lestaBoxes.totalPrice, 1170.0, 1e-9, "LESTA boxes");
    expect(lestaBoxes.purchased == 30, "LESTA box count");

    const auto wgBoxes = boxes.calculate(wot::Region::WG, 30);
    expectNear(wgBoxes.totalPrice, 40.15, 1e-9, "WG 30-pack optimization");
    expect(wgBoxes.purchased == 30, "WG box exact count");

    // A deliberately non-greedy config: buying 6 should prefer two 3-packs over a 5-pack + single.
    auto custom = rates;
    custom.wg.boxPackages = {{5, 9.0}, {3, 4.0}};
    custom.wg.singleBoxFallbackPrice = 3.0;
    wot::BoxOptimizer nonGreedy(custom);
    const auto optimized = nonGreedy.calculate(wot::Region::WG, 6);
    expectNear(optimized.totalPrice, 8.0, 1e-9, "non-greedy optimizer");

    std::cout << "All core tests passed.\n";
    return 0;
}
