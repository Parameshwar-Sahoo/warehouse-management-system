#include "core/Item.hpp"
#include "core/StorageBay.hpp"
#include "core/WarehouseZone.hpp"
#include "core/InventoryManager.hpp"
#include "core/OrderProcessor.hpp"

#include <iostream>
#include <cassert>
#include <cstring>

void runInventoryTests() {
    std::cout << "[TEST] Running Inventory Domain Unit Tests...\n";

    // Test 1: StorageBay capacity & state
    {
        wms::StorageBay bay(1, "ZONE-A", 50.0f);
        assert(bay.getState() == wms::BayState::EMPTY);
        assert(!bay.isOccupied());

        wms::Item heavyItem("SKU-HEAVY", "Machinery", wms::ItemCategory::GENERAL, 60.0f, 1);
        assert(!bay.canAccommodate(heavyItem)); // Exceeds weight

        wms::Item fitItem("SKU-FIT", "Small Sensor", wms::ItemCategory::GENERAL, 10.0f, 1);
        assert(bay.canAccommodate(fitItem));
        assert(bay.depositItem(fitItem));
        assert(bay.isOccupied());
        assert(bay.getState() == wms::BayState::OCCUPIED);

        auto retrieved = bay.retrieveItem();
        assert(retrieved.has_value());
        assert(retrieved->sku == "SKU-FIT");
        assert(!bay.isOccupied());
        assert(bay.getState() == wms::BayState::EMPTY);
    }

    // Test 2: InventoryManager Automatic Scan Intake
    {
        wms::InventoryManager inv;
        wms_scan_event_t scan{};
        scan.event_id = 1;
        std::strncpy(scan.barcode, "SKU-ELEC-401", sizeof(scan.barcode) - 1);
        scan.weight_kg = 15.0f;
        scan.source_gate_id = 1;

        auto bayId = inv.intakeItemFromScan(scan);
        assert(bayId.has_value());
        assert(*bayId >= 1 && *bayId <= 8); // Should be assigned to Zone A (Electronics, bays 1-8)

        auto bay = inv.getBay(*bayId);
        assert(bay != nullptr);
        assert(bay->isOccupied());
        assert(bay->getItem()->sku == "SKU-ELEC-401");
    }

    // Test 3: Order Fulfillment
    {
        wms::InventoryManager inv;
        wms_scan_event_t scan{};
        scan.event_id = 2;
        std::strncpy(scan.barcode, "SKU-GEN-999", sizeof(scan.barcode) - 1);
        scan.weight_kg = 20.0f;
        scan.source_gate_id = 1;
        inv.intakeItemFromScan(scan);

        wms::OrderProcessor op(inv);
        wms::Order order("ORD-TEST-1", "Client X", {{"SKU-GEN-999", 1, 0}});
        op.submitOrder(order);
        assert(op.getPendingOrders().size() == 1);

        bool success = op.processOrder("ORD-TEST-1");
        assert(success);
        assert(op.getFulfilledCount() == 1);
        assert(op.getPendingOrders().empty());
    }

    std::cout << "[PASS] Inventory Domain Unit Tests Passed!\n";
}
