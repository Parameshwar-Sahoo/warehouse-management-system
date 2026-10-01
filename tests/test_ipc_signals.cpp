#include "ipc/SharedMemoryState.hpp"
#include "ipc/SignalDispatcher.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

void runIpcTests() {
    std::cout << "[TEST] Running IPC & System Programming Tests...\n";

    // Test 1: POSIX Shared Memory Master/Client communication
    {
        wms::SharedMemoryState masterShm("/test_wms_shm");
        assert(masterShm.initialize(true));

        masterShm.update(42, 3, 100, 75.5f, "SKU-TEST-99");

        wms::SharedMemoryState clientShm("/test_wms_shm");
        assert(clientShm.initialize(false));

        wms::WmsSharedTelemetry telem{};
        assert(clientShm.readTelemetry(telem));
        assert(telem.totalScansReceived == 42);
        assert(telem.activeBayLocks == 3);
        assert(telem.totalItemsStored == 100);
        assert(telem.occupancyRate > 75.0f);
        assert(std::strcmp(telem.lastScannedBarcode, "SKU-TEST-99") == 0);

        clientShm.cleanup();
        masterShm.cleanup();
    }

    // Test 2: Signal Dispatcher initialization
    {
        wms::SignalDispatcher::initialize();
        assert(!wms::SignalDispatcher::isShutdownRequested());
        assert(!wms::SignalDispatcher::isReloadRequested());
    }

    std::cout << "[PASS] IPC & System Programming Tests Passed!\n";
}

// Forward declarations of other test suites
void runInventoryTests();
void runDeviceDriverTests();

int main() {
    std::cout << "====================================================\n";
    std::cout << "      WMS SYSTEM COMPREHENSIVE TEST SUITE           \n";
    std::cout << "====================================================\n";

    runInventoryTests();
    runDeviceDriverTests();
    runIpcTests();

    std::cout << "\n>>> ALL 3 TEST SUITES PASSED (100% SUCCESS) <<<\n";
    return 0;
}
