#pragma once

#include "StorageBay.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace wms {

class WarehouseZone {
public:
    WarehouseZone(std::string zoneId, std::string zoneName, ItemCategory allowedCategory);

    const std::string& getId() const noexcept { return zoneId_; }
    const std::string& getName() const noexcept { return zoneName_; }
    ItemCategory getAllowedCategory() const noexcept { return allowedCategory_; }

    void addBay(std::shared_ptr<StorageBay> bay);
    std::shared_ptr<StorageBay> getBay(uint32_t bayId);
    std::vector<std::shared_ptr<StorageBay>> getAllBays() const;

    size_t getTotalBays() const noexcept { return bays_.size(); }
    size_t getOccupiedBays() const noexcept;
    float getOccupancyPercentage() const noexcept;

private:
    std::string zoneId_;
    std::string zoneName_;
    ItemCategory allowedCategory_;
    std::map<uint32_t, std::shared_ptr<StorageBay>> bays_;
};

} // namespace wms
