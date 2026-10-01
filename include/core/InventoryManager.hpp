#pragma once

#include "Item.hpp"
#include "StorageBay.hpp"
#include "WarehouseZone.hpp"
#include "driver/wms_ioctl.h"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <optional>

namespace wms {

class InventoryManager {
public:
    InventoryManager();

    void addZone(std::shared_ptr<WarehouseZone> zone);
    void addBay(std::shared_ptr<StorageBay> bay);

    std::shared_ptr<StorageBay> getBay(uint32_t bayId);
    std::shared_ptr<WarehouseZone> getZone(const std::string& zoneId);

    std::vector<std::shared_ptr<StorageBay>> getAllBays() const;
    std::vector<std::shared_ptr<WarehouseZone>> getAllZones() const;

    /**
     * Ingest an item from a physical/simulated hardware scan event
     * @return allocated bay ID if successful, std::nullopt if no space or error
     */
    std::optional<uint32_t> intakeItemFromScan(const wms_scan_event_t& scan);

    /**
     * Dispatch an item by SKU and quantity
     * @return true if fulfilled, false if insufficient stock
     */
    bool dispatchItem(const std::string& sku, int quantity, std::vector<uint32_t>& outClearedBays);

    /**
     * Set electromagnetic lock state on bay
     */
    bool setBayLock(uint32_t bayId, bool lock);

    size_t getTotalBays() const;
    size_t getOccupiedBays() const;
    float getWarehouseOccupancyRate() const;
    size_t getTotalItemsStored() const;

    /* Initialize default layout with standard industrial zones */
    void initializeDefaultLayout();

private:
    std::map<std::string, std::shared_ptr<WarehouseZone>> zones_;
    std::map<uint32_t, std::shared_ptr<StorageBay>> bays_;
    mutable std::mutex mutex_;

    ItemCategory deduceCategoryFromBarcode(const std::string& barcode) const;
    std::string deduceItemNameFromBarcode(const std::string& barcode) const;
};

} // namespace wms
