#include "core/InventoryManager.hpp"
#include "core/OrderProcessor.hpp"
#include "hal/LinuxCharDevice.hpp"
#include "hal/SimulatedDevice.hpp"
#include "ipc/SharedMemoryState.hpp"
#include "ipc/SignalDispatcher.hpp"
#include "ui/TerminalDashboard.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cstring>

int main(int argc, char* argv[]) {
    bool daemonMode = false;
    bool demoMode = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--daemon") == 0 || std::strcmp(argv[i], "-d") == 0) {
            daemonMode = true;
        } else if (std::strcmp(argv[i], "--demo") == 0) {
            demoMode = true;
        }
    }

    wms::SignalDispatcher::initialize();

    std::cout << "[WMS Initialization] Starting Warehouse Management System...\n";

    // 1. Hardware Abstraction Layer Detection
    std::unique_ptr<wms::IDeviceDriver> driver;
    auto linuxDriver = std::make_unique<wms::LinuxCharDevice>();

    if (linuxDriver->connect()) {
        std::cout << "[HAL] Successfully connected to Linux Character Device: " << WMS_DEVICE_PATH << "\n";
        driver = std::move(linuxDriver);
    } else {
        std::cout << "[HAL Warning] Could not open " << WMS_DEVICE_PATH 
                  << " (Driver not loaded or insufficient permissions).\n"
                  << "[HAL] Falling back to Simulated In-Memory Character Device Driver.\n";
        auto simDriver = std::make_unique<wms::SimulatedDevice>();
        simDriver->connect();
        driver = std::move(simDriver);
    }

    // 2. Core Managers
    wms::InventoryManager inventory;
    wms::OrderProcessor orderProcessor(inventory);

    // 3. POSIX Shared Memory
    wms::SharedMemoryState shm;
    if (shm.initialize(true)) {
        std::cout << "[IPC] POSIX Shared Memory initialized at " << WMS_SHM_NAME << "\n";
    } else {
        std::cerr << "[IPC Warning] Could not initialize Shared Memory.\n";
    }

    // 4. Background Event Ingestion Worker Thread
    std::atomic<bool> workerRunning{true};
    std::vector<std::string> eventHistory;
    std::mutex eventHistoryMutex;

    auto pushEvent = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lk(eventHistoryMutex);
        eventHistory.push_back(msg);
        if (eventHistory.size() > 20) {
            eventHistory.erase(eventHistory.begin());
        }
    };

    std::thread worker([&]() {
        wms_scan_event_t event{};
        while (workerRunning && !wms::SignalDispatcher::isShutdownRequested()) {
            if (driver->readEvent(event, 200)) {
                // Process event
                if (event.source_gate_id == 1) { // Intake gate
                    auto bayOpt = inventory.intakeItemFromScan(event);
                    if (bayOpt.has_value()) {
                        uint32_t allocatedBay = bayOpt.value();
                        // Engage electromagnetic bay lock during placement
                        driver->setBayLock(allocatedBay, true);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        driver->setBayLock(allocatedBay, false);

                        std::string logMsg = "INTAKE: " + std::string(event.barcode) +
                                             " (" + std::to_string(event.weight_kg) + " kg) -> Bay #" +
                                             std::to_string(allocatedBay);
                        pushEvent(logMsg);
                    } else {
                        pushEvent("INTAKE REJECTED: Capacity full for " + std::string(event.barcode));
                    }
                } else if (event.source_gate_id == 2) { // Dispatch gate
                    std::vector<uint32_t> cleared;
                    if (inventory.dispatchItem(event.barcode, 1, cleared)) {
                        pushEvent("DISPATCH: " + std::string(event.barcode) + " retrieved from Bay #" +
                                  std::to_string(cleared.front()));
                    } else {
                        pushEvent("DISPATCH FAILED: SKU " + std::string(event.barcode) + " not in stock");
                    }
                }

                // Update shared memory telemetry
                wms_device_status_t devStatus{};
                driver->getStatus(devStatus);
                shm.update(devStatus.total_events_logged, devStatus.active_bay_locks,
                           static_cast<uint32_t>(inventory.getTotalItemsStored()),
                           inventory.getWarehouseOccupancyRate(), event.barcode);
            }
        }
    });

    wms::TerminalDashboard dashboard(inventory, orderProcessor, *driver, shm);

    // 5. Execution Modes
    if (demoMode) {
        std::cout << "\n>>> RUNNING AUTOMATED STAGE DEMONSTRATION <<<\n\n";

        // Step 1: Simulate Scans
        std::cout << "[Step 1] Ingesting test cargo into warehouse...\n";
        const char* testSkus[] = {
            "SKU-ELEC-101", "SKU-ELEC-102", "SKU-COLD-201", "SKU-COLD-202",
            "SKU-GEN-301", "SKU-GEN-302", "SKU-HAZ-401"
        };
        float weights[] = { 15.0f, 18.5f, 45.0f, 60.0f, 120.0f, 95.0f, 25.0f };

        for (int i = 0; i < 7; ++i) {
            wms_scan_event_t ev{};
            ev.event_id = i + 1;
            std::strncpy(ev.barcode, testSkus[i], sizeof(ev.barcode) - 1);
            ev.weight_kg = weights[i];
            ev.source_gate_id = 1; // Intake
            driver->simulateScan(ev);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Wait for worker thread to digest scans
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Step 2: Show Dashboard
        dashboard.renderHeader();
        dashboard.renderTelemetryBar();
        dashboard.renderBayGrid();
        {
            std::lock_guard<std::mutex> lk(eventHistoryMutex);
            dashboard.renderRecentEvents(eventHistory);
        }

        // Step 3: Test Bay Locking (IOCTL)
        std::cout << "\n[Step 2] Testing IOCTL Electromagnetic Bay Locks...\n";
        driver->setBayLock(1, true);
        driver->setBayLock(9, true);
        inventory.setBayLock(1, true);
        inventory.setBayLock(9, true);

        wms_device_status_t statusAfterLock{};
        driver->getStatus(statusAfterLock);
        std::cout << "-> Active Bay Locks reported by driver: " << statusAfterLock.active_bay_locks << "\n";

        // Step 4: Submit and Process Order
        std::cout << "\n[Step 3] Submitting Customer Order...\n";
        wms::Order ord1("ORD-8801", "Retail Hub Chicago", {
            {"SKU-ELEC-101", 1, 0},
            {"SKU-COLD-201", 1, 0}
        });
        orderProcessor.submitOrder(ord1);
        std::cout << "-> Order ORD-8801 submitted with 2 items (SKU-ELEC-101 in Bay 1, SKU-COLD-201 in Bay 9).\n";
        std::cout << "-> Attempting order fulfillment while bays are locked...\n";
        bool fulfilled = orderProcessor.processOrder("ORD-8801");
        std::cout << "-> Fulfillment blocked as expected (" << (fulfilled ? "UNEXPECTED" : "SAFETY INTERLOCK ACTIVE")
                  << "): Goods cannot be picked from locked bays!\n";

        std::cout << "-> Disengaging Bay Locks via IOCTL...\n";
        driver->setBayLock(1, false);
        driver->setBayLock(9, false);
        inventory.setBayLock(1, false);
        inventory.setBayLock(9, false);

        std::cout << "-> Retrying order fulfillment with unlocked bays...\n";
        fulfilled = orderProcessor.processOrder("ORD-8801");
        std::cout << "-> Fulfillment result: " << (fulfilled ? "SUCCESSFUL" : "FAILED") << "\n";

        // Step 5: Read Shared Memory
        std::cout << "\n[Step 4] Querying POSIX Shared Memory Telemetry...\n";
        wms::WmsSharedTelemetry telem{};
        if (shm.readTelemetry(telem)) {
            std::cout << "-> SHM PID: " << telem.processId << "\n"
                      << "-> Total Scans: " << telem.totalScansReceived << "\n"
                      << "-> Active Locks: " << telem.activeBayLocks << "\n"
                      << "-> Warehouse Occupancy: " << telem.occupancyRate << "%\n"
                      << "-> Last Barcode: " << telem.lastScannedBarcode << "\n";
        }

        std::cout << "\n>>> DEMO SEQUENCE COMPLETED SUCCESSFULLY <<<\n";
    } else if (daemonMode) {
        std::cout << "[Daemon] WMS running in background daemon mode. Press Ctrl+C to terminate.\n";
        while (!wms::SignalDispatcher::isShutdownRequested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else {
        // Interactive UI Mode
        int choice = -1;
        while (!wms::SignalDispatcher::isShutdownRequested() && choice != 0) {
            dashboard.clearScreen();
            dashboard.renderHeader();
            dashboard.renderTelemetryBar();
            dashboard.renderBayGrid();
            {
                std::lock_guard<std::mutex> lk(eventHistoryMutex);
                dashboard.renderRecentEvents(eventHistory);
            }
            dashboard.renderMenu();

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::string dummy;
                std::cin >> dummy;
                continue;
            }

            switch (choice) {
            case 1: { // Scan Intake
                std::string barcode;
                float weight;
                std::cout << "Enter Item Barcode (e.g. SKU-ELEC-501, SKU-COLD-502, SKU-GEN-503): ";
                std::cin >> barcode;
                std::cout << "Enter Item Weight (kg): ";
                std::cin >> weight;

                wms_scan_event_t ev{};
                ev.event_id = 100;
                std::strncpy(ev.barcode, barcode.c_str(), sizeof(ev.barcode) - 1);
                ev.weight_kg = weight;
                ev.source_gate_id = 1;

                driver->simulateScan(ev);
                std::cout << "[+] Scan event injected into driver. Processing...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                break;
            }
            case 2: { // Dispatch
                std::string sku;
                std::cout << "Enter SKU to dispatch: ";
                std::cin >> sku;
                wms::Order ord("ORD-" + std::to_string(rand() % 9000 + 1000), "Client Express", {{sku, 1, 0}});
                orderProcessor.submitOrder(ord);
                if (orderProcessor.processOrder(ord.orderId)) {
                    std::cout << "[+] Order fulfilled and item dispatched!\n";
                } else {
                    std::cout << "[-] Order failed: Item not in stock or bay locked.\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                break;
            }
            case 3: { // Toggle Lock
                uint32_t bayId;
                std::cout << "Enter Bay ID (1 - 24): ";
                std::cin >> bayId;
                auto bay = inventory.getBay(bayId);
                if (bay) {
                    bool newLock = !bay->isLocked();
                    driver->setBayLock(bayId, newLock);
                    inventory.setBayLock(bayId, newLock);
                    std::cout << "[+] Bay #" << bayId << " lock changed to: " << (newLock ? "LOCKED" : "UNLOCKED") << "\n";
                } else {
                    std::cout << "[-] Invalid Bay ID!\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                break;
            }
            case 4: { // Bay diagnostics
                uint32_t bayId;
                std::cout << "Enter Bay ID (1 - 24): ";
                std::cin >> bayId;
                auto bay = inventory.getBay(bayId);
                if (bay) {
                    std::cout << "\n--- Bay #" << bayId << " Diagnostics ---\n"
                              << "Zone: " << bay->getZoneId() << "\n"
                              << "Max Weight: " << bay->getMaxWeightKg() << " kg\n"
                              << "Current Weight: " << bay->getCurrentWeightKg() << " kg\n"
                              << "Status: " << (bay->isLocked() ? "LOCKED" : (bay->isOccupied() ? "OCCUPIED" : "EMPTY")) << "\n";
                    if (bay->isOccupied() && bay->getItem().has_value()) {
                        const auto& it = bay->getItem().value();
                        std::cout << "SKU: " << it.sku << "\n"
                                  << "Name: " << it.name << "\n"
                                  << "Category: " << wms::Item::categoryToString(it.category) << "\n"
                                  << "Quantity: " << it.quantity << "\n";
                    }
                    std::cout << "Press Enter to return...";
                    std::string dummy;
                    std::cin.ignore();
                    std::getline(std::cin, dummy);
                }
                break;
            }
            case 5: { // Reset buffer
                driver->resetBuffer();
                std::cout << "[+] Driver buffer reset successfully.\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
                break;
            }
            case 6: { // Read SHM
                wms::WmsSharedTelemetry telem{};
                if (shm.readTelemetry(telem)) {
                    std::cout << "\n--- POSIX Shared Memory Telemetry ---\n"
                              << "PID: " << telem.processId << "\n"
                              << "Total Scans: " << telem.totalScansReceived << "\n"
                              << "Active Bay Locks: " << telem.activeBayLocks << "\n"
                              << "Total Items: " << telem.totalItemsStored << "\n"
                              << "Occupancy Rate: " << telem.occupancyRate << "%\n"
                              << "Last Barcode: " << telem.lastScannedBarcode << "\n"
                              << "Heartbeat (epoch ms): " << telem.lastHeartbeatEpochMs << "\n"
                              << "Press Enter to return...";
                    std::string dummy;
                    std::cin.ignore();
                    std::getline(std::cin, dummy);
                }
                break;
            }
            case 7:
                // Refresh
                break;
            case 0:
                std::cout << "Exiting...\n";
                break;
            default:
                break;
            }
        }
    }

    // 6. Graceful Cleanup
    std::cout << "[WMS Shutdown] Stopping worker thread and freeing resources...\n";
    workerRunning = false;
    if (worker.joinable()) {
        worker.join();
    }

    // Release all electromagnetic locks before power down
    for (uint32_t i = 1; i <= 24; ++i) {
        driver->setBayLock(i, false);
    }
    shm.cleanup();
    driver->disconnect();

    std::cout << "[WMS Shutdown] Clean shutdown complete. Goodbye!\n";
    return 0;
}
