#include "core/StorageBay.hpp"

namespace wms {

StorageBay::StorageBay(uint32_t bayId, std::string zoneId, float maxWeightKg)
    : bayId_(bayId), zoneId_(std::move(zoneId)), maxWeightKg_(maxWeightKg) {}

BayState StorageBay::getState() const noexcept {
    if (isLocked_) return BayState::LOCKED;
    if (isOccupied_) return BayState::OCCUPIED;
    return BayState::EMPTY;
}

bool StorageBay::canAccommodate(const Item& item) const noexcept {
    if (isLocked_ || isOccupied_) return false;
    return (item.weightKg <= maxWeightKg_);
}

bool StorageBay::depositItem(const Item& item) {
    if (!canAccommodate(item)) return false;
    storedItem_ = item;
    currentWeightKg_ = item.weightKg;
    isOccupied_ = true;
    return true;
}

std::optional<Item> StorageBay::retrieveItem() {
    if (isLocked_ || !isOccupied_ || !storedItem_.has_value()) {
        return std::nullopt;
    }
    Item item = *storedItem_;
    clear();
    return item;
}

void StorageBay::clear() {
    storedItem_ = std::nullopt;
    currentWeightKg_ = 0.0f;
    isOccupied_ = false;
}

} // namespace wms
