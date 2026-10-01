#pragma once

#include "IDeviceDriver.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace wms {

class SimulatedDevice : public IDeviceDriver {
public:
    SimulatedDevice();
    ~SimulatedDevice() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return isConnected_; }

    bool readEvent(wms_scan_event_t& outEvent, int timeoutMs) override;
    bool setBayLock(uint32_t bayId, bool lock) override;
    bool getBayLock(uint32_t bayId, bool& outLocked) override;
    bool getStatus(wms_device_status_t& outStatus) override;
    bool simulateScan(const wms_scan_event_t& event) override;
    bool resetBuffer() override;

    std::string getDriverName() const override { return "SimulatedDevice (HAL In-Memory Driver)"; }

private:
    std::atomic<bool> isConnected_{false};
    std::queue<wms_scan_event_t> eventQueue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::vector<bool> bayLocks_;
    uint32_t totalEventsLogged_{0};
    uint32_t droppedEvents_{0};
};

} // namespace wms
