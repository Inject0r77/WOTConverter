#include "core/BoxOptimizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace wot {

BoxPurchaseResult BoxOptimizer::calculate(Region region, int requested) const {
    if (requested < 0) {
        throw std::invalid_argument("box count must be non-negative");
    }
    if (requested == 0) return {};

    BoxPurchaseResult result;
    result.requested = requested;

    if (region == Region::Lesta) {
        result.purchased = requested;
        result.totalPrice = requested * rates_.lesta.singleBoxPriceRub;
        result.lines.push_back({1, requested, rates_.lesta.singleBoxPriceRub});
        return result;
    }

    std::vector<BoxPackage> packages = rates_.wg.boxPackages;
    packages.push_back({1, rates_.wg.singleBoxFallbackPrice});
    packages.erase(std::remove_if(packages.begin(), packages.end(), [](const BoxPackage& p) {
        return p.count <= 0 || !std::isfinite(p.price) || p.price <= 0.0;
    }), packages.end());
    if (packages.empty()) throw std::runtime_error("no valid box packages configured");

    const int largest = std::max_element(packages.begin(), packages.end(), [](const auto& a, const auto& b) {
        return a.count < b.count;
    })->count;

    // We only need to search slightly beyond the requested count: any larger overshoot
    // can be reduced by at least one package without dropping below the target.
    const int limit = requested + largest;
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<double> best(limit + 1, inf);
    std::vector<int> prevCount(limit + 1, -1);
    std::vector<int> prevPackage(limit + 1, -1);
    best[0] = 0.0;

    for (int count = 0; count <= limit; ++count) {
        if (!std::isfinite(best[count])) continue;
        for (int i = 0; i < static_cast<int>(packages.size()); ++i) {
            const int next = count + packages[i].count;
            if (next > limit) continue;
            const double candidate = best[count] + packages[i].price;
            if (candidate + 1e-9 < best[next]) {
                best[next] = candidate;
                prevCount[next] = count;
                prevPackage[next] = i;
            }
        }
    }

    int bestCount = requested;
    for (int count = requested; count <= limit; ++count) {
        if (best[count] + 1e-9 < best[bestCount]) bestCount = count;
    }

    if (!std::isfinite(best[bestCount])) {
        throw std::runtime_error("unable to build box package combination");
    }

    std::map<int, int, std::greater<int>> lineCounts;
    int cursor = bestCount;
    while (cursor > 0) {
        const int pkgIndex = prevPackage[cursor];
        if (pkgIndex < 0) throw std::runtime_error("box optimizer reconstruction failed");
        ++lineCounts[pkgIndex];
        cursor = prevCount[cursor];
    }

    result.purchased = bestCount;
    result.totalPrice = best[bestCount];
    for (const auto& [pkgIndex, count] : lineCounts) {
        const auto& pkg = packages[pkgIndex];
        result.lines.push_back({pkg.count, count, pkg.price});
    }
    std::sort(result.lines.begin(), result.lines.end(), [](const auto& a, const auto& b) {
        return a.packageSize > b.packageSize;
    });
    return result;
}

} // namespace wot
