#pragma once

#include "core/Rates.h"
#include "core/Types.h"

namespace wot {

class BoxOptimizer {
public:
    explicit BoxOptimizer(const RatesConfig& rates) : rates_(rates) {}

    [[nodiscard]] BoxPurchaseResult calculate(Region region, int requested) const;

private:
    const RatesConfig& rates_;
};

} // namespace wot
