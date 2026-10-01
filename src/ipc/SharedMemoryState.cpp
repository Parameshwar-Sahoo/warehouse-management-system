#include "ipc/SharedMemoryState.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <iostream>

namespace wms {

SharedMemoryState::SharedMemoryState(std::string shmName)
    : shmName_(std::move(shmName)) {}

SharedMemoryState::~SharedMemoryState() {
    cleanup();
}

bool SharedMemoryState::initialize(bool createAsMaster) {
    isMaster_ = createAsMaster;

    if (createAsMaster) {
        // Unlink any stale instance first
        shm_unlink(shmName_.c_str());

        shmFd_ = shm_open(shmName_.c_str(), O_CREAT | O_RDWR, 0666);
        if (shmFd_ < 0) {
            return false;
        }

        if (ftruncate(shmFd_, sizeof(WmsSharedTelemetry)) != 0) {
            close(shmFd_);
            shmFd_ = -1;
            return false;
        }
    } else {
        shmFd_ = shm_open(shmName_.c_str(), O_RDONLY, 0666);
        if (shmFd_ < 0) {
            return false;
        }
    }

    int prot = createAsMaster ? (PROT_READ | PROT_WRITE) : PROT_READ;
    sharedData_ = static_cast<WmsSharedTelemetry*>(
        mmap(nullptr, sizeof(WmsSharedTelemetry), prot, MAP_SHARED, shmFd_, 0));

    if (sharedData_ == MAP_FAILED) {
        sharedData_ = nullptr;
        close(shmFd_);
        shmFd_ = -1;
        return false;
    }

    if (createAsMaster) {
        std::memset(sharedData_, 0, sizeof(WmsSharedTelemetry));
        sharedData_->processId = static_cast<uint32_t>(getpid());
        sharedData_->isDaemonRunning = 1;
        sharedData_->lastHeartbeatEpochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                               std::chrono::system_clock::now().time_since_epoch())
                                               .count();
    }

    return true;
}

void SharedMemoryState::update(uint32_t scans, uint32_t locks, uint32_t items,
                              float occupancy, const std::string& lastBarcode) {
    if (!sharedData_ || !isMaster_) return;

    sharedData_->totalScansReceived = scans;
    sharedData_->activeBayLocks = locks;
    sharedData_->totalItemsStored = items;
    sharedData_->occupancyRate = occupancy;

    std::strncpy(sharedData_->lastScannedBarcode, lastBarcode.c_str(), sizeof(sharedData_->lastScannedBarcode) - 1);
    sharedData_->lastScannedBarcode[sizeof(sharedData_->lastScannedBarcode) - 1] = '\0';

    sharedData_->lastHeartbeatEpochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                           std::chrono::system_clock::now().time_since_epoch())
                                           .count();
}

bool SharedMemoryState::readTelemetry(WmsSharedTelemetry& outTelemetry) {
    if (!sharedData_) return false;
    outTelemetry = *sharedData_;
    return true;
}

void SharedMemoryState::cleanup() {
    if (sharedData_) {
        if (isMaster_) {
            sharedData_->isDaemonRunning = 0;
        }
        munmap(sharedData_, sizeof(WmsSharedTelemetry));
        sharedData_ = nullptr;
    }
    if (shmFd_ >= 0) {
        close(shmFd_);
        shmFd_ = -1;
    }
    if (isMaster_) {
        shm_unlink(shmName_.c_str());
        isMaster_ = false;
    }
}

} // namespace wms
