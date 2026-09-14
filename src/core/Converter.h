#pragma once

#include "core/Rates.h"
#include "core/Types.h"

namespace wot {

class Converter {
public:
    explicit Converter(RatesConfig rates);

    [[nodiscard]] const RatesConfig& rates() const noexcept { return rates_; }
    [[nodiscard]] ConversionResult convert(const ConversionInput& input) const;

    [[nodiscard]] double lestaRubFromGold(double gold) const;
    [[nodiscard]] double lestaGoldFromRub(double rub) const;

private:
    RatesConfig rates_;
};

} // namespace wot
