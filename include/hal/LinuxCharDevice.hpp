#pragma once

#include "IDeviceDriver.hpp"
#include <string>
#include <atomic>

namespace wms {

class LinuxCharDevice : public IDeviceDriver {
public:
    explicit LinuxCharDevice(std::string devicePath = WMS_DEVICE_PATH);
    ~LinuxCharDevice() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return isConnected_; }

    bool readEvent(wms_scan_event_t& outEvent, int timeoutMs) override;
    bool setBayLock(uint32_t bayId, bool lock) override;
    bool getBayLock(uint32_t bayId, bool& outLocked) override;
    bool getStatus(wms_device_status_t& outStatus) override;
    bool simulateScan(const wms_scan_event_t& event) override;
    bool resetBuffer() override;

    std::string getDriverName() const override { return "LinuxCharDevice (" + devicePath_ + ")"; }

private:
    std::string devicePath_;
    int fd_{-1};
    std::atomic<bool> isConnected_{false};
};

} // namespace wms
