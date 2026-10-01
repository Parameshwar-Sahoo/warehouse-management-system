#pragma once

#include "Item.hpp"
#include <cstdint>
#include <string>
#include <optional>

namespace wms {

enum class BayState {
    EMPTY,
    OCCUPIED,
    LOCKED,
    MAINTENANCE
};

class StorageBay {
public:
    StorageBay(uint32_t bayId, std::string zoneId, float maxWeightKg);

    uint32_t getId() const noexcept { return bayId_; }
    const std::string& getZoneId() const noexcept { return zoneId_; }
    float getMaxWeightKg() const noexcept { return maxWeightKg_; }
    float getCurrentWeightKg() const noexcept { return currentWeightKg_; }
    bool isLocked() const noexcept { return isLocked_; }
    bool isOccupied() const noexcept { return isOccupied_; }
    BayState getState() const noexcept;

    const std::optional<Item>& getItem() const noexcept { return storedItem_; }

    void setLock(bool locked) noexcept { isLocked_ = locked; }

    bool canAccommodate(const Item& item) const noexcept;
    bool depositItem(const Item& item);
    std::optional<Item> retrieveItem();
    void clear();

private:
    uint32_t bayId_;
    std::string zoneId_;
    float maxWeightKg_;
    float currentWeightKg_{0.0f};
    bool isLocked_{false};
    bool isOccupied_{false};
    std::optional<Item> storedItem_{std::nullopt};
};

} // namespace wms
