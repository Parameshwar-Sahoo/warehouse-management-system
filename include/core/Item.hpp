#pragma once

#include <string>
#include <cstdint>
#include <chrono>

namespace wms {

enum class ItemCategory {
    GENERAL,
    ELECTRONICS,
    PERISHABLE,
    HAZARDOUS,
    FRAGILE
};

struct Item {
    std::string sku;
    std::string name;
    ItemCategory category;
    float weightKg;
    int quantity;
    uint64_t registeredTimestamp;

    Item()
        : sku(""), name(""), category(ItemCategory::GENERAL), weightKg(0.0f),
          quantity(0), registeredTimestamp(0) {}

    Item(std::string s, std::string n, ItemCategory cat, float w, int q)
        : sku(std::move(s)), name(std::move(n)), category(cat), weightKg(w),
          quantity(q),
          registeredTimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count()) {}

    static std::string categoryToString(ItemCategory cat) {
        switch (cat) {
        case ItemCategory::GENERAL: return "General";
        case ItemCategory::ELECTRONICS: return "Electronics";
        case ItemCategory::PERISHABLE: return "Perishable";
        case ItemCategory::HAZARDOUS: return "Hazardous";
        case ItemCategory::FRAGILE: return "Fragile";
        default: return "Unknown";
        }
    }
};

} // namespace wms
