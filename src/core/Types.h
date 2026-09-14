#pragma once

#include <string>
#include <vector>

namespace wot {

enum class Region {
    Lesta,
    WG
};

enum class SourceKind {
    Money,
    Gold,
    Credits,
    FreeXp
};

struct Modifiers {
    bool promoFreeXp = false;
    bool premiumShopDiscount = false;
};

struct ConversionInput {
    Region region = Region::Lesta;
    SourceKind source = SourceKind::Money;
    double value = 0.0;
    Modifiers modifiers{};
};

struct ConversionResult {
    double money = 0.0;
    double gold = 0.0;
    double credits = 0.0;
    double freeXp = 0.0;
    double premiumDays = 0.0;
};

struct BoxPackage {
    int count = 0;
    double price = 0.0;
};

struct BoxPurchaseLine {
    int packageSize = 0;
    int packageCount = 0;
    double packagePrice = 0.0;
};

struct BoxPurchaseResult {
    int requested = 0;
    int purchased = 0;
    double totalPrice = 0.0;
    std::vector<BoxPurchaseLine> lines;
};

} // namespace wot
