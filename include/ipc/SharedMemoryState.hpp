#pragma once

#include <string>
#include <cstdint>
#include <atomic>

namespace wms {

#define WMS_SHM_NAME "/wms_telemetry_shm"

#pragma pack(push, 1)
struct WmsSharedTelemetry {
    uint32_t processId;
    uint32_t totalScansReceived;
    uint32_t activeBayLocks;
    uint32_t totalItemsStored;
    float    occupancyRate;
    char     lastScannedBarcode[32];
    uint64_t lastHeartbeatEpochMs;
    uint8_t  isDaemonRunning;
};
#pragma pack(pop)

class SharedMemoryState {
public:
    explicit SharedMemoryState(std::string shmName = WMS_SHM_NAME);
    ~SharedMemoryState();

    bool initialize(bool createAsMaster);
    void update(uint32_t scans, uint32_t locks, uint32_t items, float occupancy, const std::string& lastBarcode);
    bool readTelemetry(WmsSharedTelemetry& outTelemetry);
    void cleanup();

private:
    std::string shmName_;
    int shmFd_{-1};
    WmsSharedTelemetry* sharedData_{nullptr};
    bool isMaster_{false};
};

} // namespace wms
