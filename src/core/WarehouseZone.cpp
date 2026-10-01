#include "core/WarehouseZone.hpp"

namespace wms {

WarehouseZone::WarehouseZone(std::string zoneId, std::string zoneName, ItemCategory allowedCategory)
    : zoneId_(std::move(zoneId)), zoneName_(std::move(zoneName)), allowedCategory_(allowedCategory) {}

void WarehouseZone::addBay(std::shared_ptr<StorageBay> bay) {
    if (bay) {
        bays_[bay->getId()] = bay;
    }
}

std::shared_ptr<StorageBay> WarehouseZone::getBay(uint32_t bayId) {
    auto it = bays_.find(bayId);
    if (it != bays_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<StorageBay>> WarehouseZone::getAllBays() const {
    std::vector<std::shared_ptr<StorageBay>> result;
    result.reserve(bays_.size());
    for (const auto& [id, bay] : bays_) {
        result.push_back(bay);
    }
    return result;
}

size_t WarehouseZone::getOccupiedBays() const noexcept {
    size_t count = 0;
    for (const auto& [id, bay] : bays_) {
        if (bay->isOccupied()) count++;
    }
    return count;
}

float WarehouseZone::getOccupancyPercentage() const noexcept {
    if (bays_.empty()) return 0.0f;
    return (static_cast<float>(getOccupiedBays()) / static_cast<float>(bays_.size())) * 100.0f;
}

} // namespace wms
