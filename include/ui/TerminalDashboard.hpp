#pragma once

#include "core/InventoryManager.hpp"
#include "core/OrderProcessor.hpp"
#include "hal/IDeviceDriver.hpp"
#include "ipc/SharedMemoryState.hpp"
#include <string>
#include <vector>

namespace wms {

class TerminalDashboard {
public:
    TerminalDashboard(InventoryManager& inventory, OrderProcessor& orders,
                      IDeviceDriver& driver, SharedMemoryState& shm);

    void renderHeader() const;
    void renderBayGrid() const;
    void renderTelemetryBar() const;
    void renderRecentEvents(const std::vector<std::string>& events) const;
    void renderMenu() const;

    void clearScreen() const;

private:
    InventoryManager& inventory_;
    OrderProcessor& orders_;
    IDeviceDriver& driver_;
    SharedMemoryState& shm_;
};

} // namespace wms
