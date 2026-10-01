#include "hal/SimulatedDevice.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <thread>

void runDeviceDriverTests() {
    std::cout << "[TEST] Running Device Driver & HAL Unit Tests...\n";

    wms::SimulatedDevice dev;
    assert(dev.connect());
    assert(dev.isConnected());

    // Test 1: Empty read with timeout
    wms_scan_event_t ev{};
    bool readSuccess = dev.readEvent(ev, 50);
    assert(!readSuccess);

    // Test 2: Injected scan event
    wms_scan_event_t scan1{};
    scan1.event_id = 101;
    std::strncpy(scan1.barcode, "BARCODE-001", sizeof(scan1.barcode) - 1);
    scan1.weight_kg = 5.5f;
    scan1.source_gate_id = 1;

    assert(dev.simulateScan(scan1));

    wms_device_status_t status{};
    assert(dev.getStatus(status));
    assert(status.buffer_occupancy == 1);
    assert(status.total_events_logged == 1);

    // Test 3: Reading the injected event
    assert(dev.readEvent(ev, 100));
    assert(ev.event_id == 101);
    assert(std::strcmp(ev.barcode, "BARCODE-001") == 0);

    // Test 4: Bay lock actuation
    assert(dev.setBayLock(5, true));
    bool isLocked = false;
    assert(dev.getBayLock(5, isLocked));
    assert(isLocked == true);

    assert(dev.setBayLock(5, false));
    assert(dev.getBayLock(5, isLocked));
    assert(isLocked == false);

    // Test 5: Buffer overflow drop handling
    dev.resetBuffer();
    for (int i = 0; i < WMS_BUFFER_CAPACITY + 10; ++i) {
        wms_scan_event_t overflowEv{};
        overflowEv.event_id = i;
        dev.simulateScan(overflowEv);
    }
    assert(dev.getStatus(status));
    assert(status.buffer_occupancy == WMS_BUFFER_CAPACITY);
    assert(status.dropped_events == 10);

    dev.disconnect();
    assert(!dev.isConnected());

    std::cout << "[PASS] Device Driver & HAL Tests Passed!\n";
}
