#include "hal/SimulatedDevice.hpp"
#include <chrono>

namespace wms {

SimulatedDevice::SimulatedDevice()
    : bayLocks_(WMS_MAX_BAYS, false) {}

SimulatedDevice::~SimulatedDevice() {
    disconnect();
}

bool SimulatedDevice::connect() {
    isConnected_ = true;
    return true;
}

void SimulatedDevice::disconnect() {
    isConnected_ = false;
    cv_.notify_all();
}

bool SimulatedDevice::readEvent(wms_scan_event_t& outEvent, int timeoutMs) {
    if (!isConnected_) return false;

    std::unique_lock<std::mutex> lock(mutex_);

    if (timeoutMs < 0) {
        cv_.wait(lock, [this]() { return !eventQueue_.empty() || !isConnected_; });
    } else {
        cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                     [this]() { return !eventQueue_.empty() || !isConnected_; });
    }

    if (!isConnected_ || eventQueue_.empty()) {
        return false;
    }

    outEvent = eventQueue_.front();
    eventQueue_.pop();
    return true;
}

bool SimulatedDevice::setBayLock(uint32_t bayId, bool lock) {
    if (bayId >= WMS_MAX_BAYS) return false;
    std::lock_guard<std::mutex> lk(mutex_);
    bayLocks_[bayId] = lock;
    return true;
}

bool SimulatedDevice::getBayLock(uint32_t bayId, bool& outLocked) {
    if (bayId >= WMS_MAX_BAYS) return false;
    std::lock_guard<std::mutex> lk(mutex_);
    outLocked = bayLocks_[bayId];
    return true;
}

bool SimulatedDevice::getStatus(wms_device_status_t& outStatus) {
    std::lock_guard<std::mutex> lk(mutex_);
    outStatus.total_events_logged = totalEventsLogged_;
    outStatus.buffer_occupancy = static_cast<uint32_t>(eventQueue_.size());
    outStatus.buffer_capacity = WMS_BUFFER_CAPACITY;
    outStatus.dropped_events = droppedEvents_;

    uint32_t activeLocks = 0;
    for (bool b : bayLocks_) {
        if (b) activeLocks++;
    }
    outStatus.active_bay_locks = activeLocks;
    outStatus.device_ready = isConnected_ ? 1 : 0;
    return true;
}

bool SimulatedDevice::simulateScan(const wms_scan_event_t& event) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (eventQueue_.size() >= WMS_BUFFER_CAPACITY) {
        droppedEvents_++;
        return false;
    }

    wms_scan_event_t ev = event;
    if (ev.timestamp_ns == 0) {
        ev.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
    }

    eventQueue_.push(ev);
    totalEventsLogged_++;
    cv_.notify_one();
    return true;
}

bool SimulatedDevice::resetBuffer() {
    std::lock_guard<std::mutex> lk(mutex_);
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
    droppedEvents_ = 0;
    return true;
}

} // namespace wms
