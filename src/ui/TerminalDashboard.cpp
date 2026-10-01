#include "ui/TerminalDashboard.hpp"
#include <iostream>
#include <iomanip>

namespace wms {

// ANSI Colors
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BG_BLUE "\033[44m"

TerminalDashboard::TerminalDashboard(InventoryManager& inventory, OrderProcessor& orders,
                                     IDeviceDriver& driver, SharedMemoryState& shm)
    : inventory_(inventory), orders_(orders), driver_(driver), shm_(shm) {}

void TerminalDashboard::clearScreen() const {
    std::cout << "\033[2J\033[1;1H";
}

void TerminalDashboard::renderHeader() const {
    std::cout << ANSI_BOLD << ANSI_CYAN
              << "========================================================================================\n"
              << "          SMART WAREHOUSE MANAGEMENT SYSTEM (Linux CharDev & Modern C++)               \n"
              << "========================================================================================\n"
              << ANSI_RESET;
}

void TerminalDashboard::renderTelemetryBar() const {
    wms_device_status_t status{};
    driver_.getStatus(status);

    float occ = inventory_.getWarehouseOccupancyRate();
    size_t totalStored = inventory_.getTotalItemsStored();

    std::cout << ANSI_BOLD << "[HARDWARE TELEMETRY & SYSTEM STATE]" << ANSI_RESET << "\n";
    std::cout << " Driver: " << ANSI_YELLOW << driver_.getDriverName() << ANSI_RESET
              << " | Connected: " << (driver_.isConnected() ? (ANSI_GREEN "YES" ANSI_RESET) : (ANSI_RED "NO" ANSI_RESET))
              << " | Active Locks: " << ANSI_RED << status.active_bay_locks << ANSI_RESET
              << " | Total Scans: " << ANSI_CYAN << status.total_events_logged << ANSI_RESET << "\n";
    std::cout << " Buffer Occupancy: " << status.buffer_occupancy << "/" << status.buffer_capacity
              << " | Dropped Events: " << (status.dropped_events > 0 ? ANSI_RED : ANSI_GREEN)
              << status.dropped_events << ANSI_RESET
              << " | Total Items: " << totalStored
              << " | Wh Occupancy: " << std::fixed << std::setprecision(1) << occ << "%\n";
    std::cout << " Orders Fulfilled: " << ANSI_GREEN << orders_.getFulfilledCount() << ANSI_RESET
              << " | Pending Orders: " << orders_.getPendingOrders().size() << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void TerminalDashboard::renderBayGrid() const {
    std::cout << ANSI_BOLD << "[STORAGE BAY RACKS LAYOUT - 24 BAYS]" << ANSI_RESET << "\n";
    std::cout << " Legend: "
              << ANSI_GREEN << "[FREE] " << ANSI_RESET
              << ANSI_YELLOW << "[OCCUPIED] " << ANSI_RESET
              << ANSI_RED << "[LOCKED] " << ANSI_RESET << "\n\n";

    auto bays = inventory_.getAllBays();

    for (size_t i = 0; i < bays.size(); ++i) {
        const auto& bay = bays[i];
        std::cout << "[Bay " << std::setw(2) << bay->getId() << " ";

        if (bay->isLocked()) {
            std::cout << ANSI_RED << "LOCKED  " << ANSI_RESET;
        } else if (bay->isOccupied()) {
            std::cout << ANSI_YELLOW << "OCCUPIED" << ANSI_RESET;
        } else {
            std::cout << ANSI_GREEN << "EMPTY   " << ANSI_RESET;
        }

        std::cout << "] ";

        if ((i + 1) % 4 == 0) {
            std::cout << "\n";
        }
    }
    std::cout << "\n----------------------------------------------------------------------------------------\n";
}

void TerminalDashboard::renderRecentEvents(const std::vector<std::string>& events) const {
    std::cout << ANSI_BOLD << "[LIVE HARDWARE SCAN STREAM / EVENT LOG]" << ANSI_RESET << "\n";
    if (events.empty()) {
        std::cout << "  (No events recorded yet. Ready for scans...)\n";
    } else {
        size_t start = (events.size() > 5) ? events.size() - 5 : 0;
        for (size_t i = start; i < events.size(); ++i) {
            std::cout << "  * " << events[i] << "\n";
        }
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void TerminalDashboard::renderMenu() const {
    std::cout << ANSI_BOLD << "[OPERATIONS MENU]" << ANSI_RESET << "\n"
              << " 1) Scan Intake (Trigger Barcode/RFID scan)\n"
              << " 2) Dispatch Order (Pick item from bay)\n"
              << " 3) Toggle Electromagnetic Bay Lock (ioctl)\n"
              << " 4) View Bay Detailed Diagnostics\n"
              << " 5) Reset Driver Ring Buffer & Stats (ioctl)\n"
              << " 6) Read POSIX Shared Memory Telemetry\n"
              << " 7) Refresh Dashboard\n"
              << " 0) Exit / Graceful Shutdown\n"
              << "Select option: ";
}

} // namespace wms
