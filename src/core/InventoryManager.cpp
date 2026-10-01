#include "core/InventoryManager.hpp"
#include <iostream>
#include <algorithm>

namespace wms {

InventoryManager::InventoryManager() {
    initializeDefaultLayout();
}

void InventoryManager::initializeDefaultLayout() {
    // Zone A: Electronics (Fragile / High Value) - 8 Bays
    auto zoneA = std::make_shared<WarehouseZone>("ZONE-A", "Electronics & High-Tech", ItemCategory::ELECTRONICS);
    // Zone B: Cold Storage (Perishable) - 8 Bays
    auto zoneB = std::make_shared<WarehouseZone>("ZONE-B", "Cold Storage", ItemCategory::PERISHABLE);
    // Zone C: General Goods (Heavy / Ambient) - 8 Bays
    auto zoneC = std::make_shared<WarehouseZone>("ZONE-C", "General Ambient", ItemCategory::GENERAL);

    addZone(zoneA);
    addZone(zoneB);
    addZone(zoneC);

    // Populate 24 bays (Bays 1-8 -> Zone A, Bays 9-16 -> Zone B, Bays 17-24 -> Zone C)
    for (uint32_t i = 1; i <= 8; ++i) {
        auto bay = std::make_shared<StorageBay>(i, "ZONE-A", 100.0f);
        addBay(bay);
        zoneA->addBay(bay);
    }
    for (uint32_t i = 9; i <= 16; ++i) {
        auto bay = std::make_shared<StorageBay>(i, "ZONE-B", 250.0f);
        addBay(bay);
        zoneB->addBay(bay);
    }
    for (uint32_t i = 17; i <= 24; ++i) {
        auto bay = std::make_shared<StorageBay>(i, "ZONE-C", 500.0f);
        addBay(bay);
        zoneC->addBay(bay);
    }
}

void InventoryManager::addZone(std::shared_ptr<WarehouseZone> zone) {
    if (zone) {
        std::lock_guard<std::mutex> lock(mutex_);
        zones_[zone->getId()] = zone;
    }
}

void InventoryManager::addBay(std::shared_ptr<StorageBay> bay) {
    if (bay) {
        std::lock_guard<std::mutex> lock(mutex_);
        bays_[bay->getId()] = bay;
    }
}

std::shared_ptr<StorageBay> InventoryManager::getBay(uint32_t bayId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bays_.find(bayId);
    return (it != bays_.end()) ? it->second : nullptr;
}

std::shared_ptr<WarehouseZone> InventoryManager::getZone(const std::string& zoneId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = zones_.find(zoneId);
    return (it != zones_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<StorageBay>> InventoryManager::getAllBays() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<StorageBay>> list;
    list.reserve(bays_.size());
    for (const auto& [id, bay] : bays_) {
        list.push_back(bay);
    }
    return list;
}

std::vector<std::shared_ptr<WarehouseZone>> InventoryManager::getAllZones() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<WarehouseZone>> list;
    list.reserve(zones_.size());
    for (const auto& [id, zone] : zones_) {
        list.push_back(zone);
    }
    return list;
}

ItemCategory InventoryManager::deduceCategoryFromBarcode(const std::string& barcode) const {
    if (barcode.find("ELEC") != std::string::npos || barcode.find("TECH") != std::string::npos) {
        return ItemCategory::ELECTRONICS;
    }
    if (barcode.find("COLD") != std::string::npos || barcode.find("FOOD") != std::string::npos) {
        return ItemCategory::PERISHABLE;
    }
    if (barcode.find("HAZ") != std::string::npos || barcode.find("CHEM") != std::string::npos) {
        return ItemCategory::HAZARDOUS;
    }
    return ItemCategory::GENERAL;
}

std::string InventoryManager::deduceItemNameFromBarcode(const std::string& barcode) const {
    if (barcode.find("ELEC") != std::string::npos) return "Server Motherboard Unit";
    if (barcode.find("COLD") != std::string::npos) return "Perishable Vaccine Pack";
    if (barcode.find("HAZ") != std::string::npos) return "Industrial Solvent Canister";
    return "Standard Pallet Cargo";
}

std::optional<uint32_t> InventoryManager::intakeItemFromScan(const wms_scan_event_t& scan) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string barcodeStr(scan.barcode);
    ItemCategory cat = deduceCategoryFromBarcode(barcodeStr);
    std::string itemName = deduceItemNameFromBarcode(barcodeStr);
    float weight = (scan.weight_kg > 0.0f) ? scan.weight_kg : 12.5f;

    Item item(barcodeStr, itemName, cat, weight, 1);

    // Strategy 1: Find empty bay in the designated category zone
    std::shared_ptr<StorageBay> selectedBay = nullptr;
    for (const auto& [zId, zone] : zones_) {
        if (zone->getAllowedCategory() == cat) {
            for (const auto& bay : zone->getAllBays()) {
                if (bay->canAccommodate(item)) {
                    selectedBay = bay;
                    break;
                }
            }
        }
        if (selectedBay) break;
    }

    // Strategy 2: If primary zone is full, search any bay with sufficient weight capacity
    if (!selectedBay) {
        for (const auto& [id, bay] : bays_) {
            if (bay->canAccommodate(item)) {
                selectedBay = bay;
                break;
            }
        }
    }

    if (!selectedBay) {
        return std::nullopt; // Warehouse is full or exceeds capacity
    }

    selectedBay->depositItem(item);
    return selectedBay->getId();
}

bool InventoryManager::dispatchItem(const std::string& sku, int quantity, std::vector<uint32_t>& outClearedBays) {
    std::lock_guard<std::mutex> lock(mutex_);
    outClearedBays.clear();

    int remaining = quantity;
    for (auto& [id, bay] : bays_) {
        if (bay->isOccupied() && !bay->isLocked()) {
            const auto& it = bay->getItem();
            if (it && it->sku == sku) {
                bay->clear();
                outClearedBays.push_back(id);
                remaining--;
                if (remaining <= 0) break;
            }
        }
    }

    return (remaining <= 0);
}

bool InventoryManager::setBayLock(uint32_t bayId, bool lock) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = bays_.find(bayId);
    if (it != bays_.end()) {
        it->second->setLock(lock);
        return true;
    }
    return false;
}

size_t InventoryManager::getTotalBays() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bays_.size();
}

size_t InventoryManager::getOccupiedBays() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, bay] : bays_) {
        if (bay->isOccupied()) count++;
    }
    return count;
}

float InventoryManager::getWarehouseOccupancyRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bays_.empty()) return 0.0f;
    size_t occ = 0;
    for (const auto& [id, bay] : bays_) {
        if (bay->isOccupied()) occ++;
    }
    return (static_cast<float>(occ) / static_cast<float>(bays_.size())) * 100.0f;
}

size_t InventoryManager::getTotalItemsStored() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [id, bay] : bays_) {
        if (bay->isOccupied() && bay->getItem().has_value()) {
            total += bay->getItem()->quantity;
        }
    }
    return total;
}

} // namespace wms
