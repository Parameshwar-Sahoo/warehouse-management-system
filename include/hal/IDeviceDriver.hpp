#pragma once

#include "driver/wms_ioctl.h"
#include <string>
#include <cstdint>

namespace wms {

class IDeviceDriver {
public:
    virtual ~IDeviceDriver() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    /**
     * Read next scan event from driver (blocking up to timeoutMs, or -1 for infinite)
     */
    virtual bool readEvent(wms_scan_event_t& outEvent, int timeoutMs) = 0;

    /**
     * Actuate electromagnetic lock on bay
     */
    virtual bool setBayLock(uint32_t bayId, bool lock) = 0;

    /**
     * Query lock status on bay
     */
    virtual bool getBayLock(uint32_t bayId, bool& outLocked) = 0;

    /**
     * Fetch driver telemetry & statistics
     */
    virtual bool getStatus(wms_device_status_t& outStatus) = 0;

    /**
     * Inject a simulated scan event
     */
    virtual bool simulateScan(const wms_scan_event_t& event) = 0;

    /**
     * Reset driver buffer
     */
    virtual bool resetBuffer() = 0;

    virtual std::string getDriverName() const = 0;
};

} // namespace wms
