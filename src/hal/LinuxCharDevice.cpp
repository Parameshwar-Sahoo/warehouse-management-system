#include "hal/LinuxCharDevice.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace wms {

LinuxCharDevice::LinuxCharDevice(std::string devicePath)
    : devicePath_(std::move(devicePath)) {}

LinuxCharDevice::~LinuxCharDevice() {
    disconnect();
}

bool LinuxCharDevice::connect() {
    if (isConnected_) return true;

    fd_ = open(devicePath_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ < 0) {
        // Try without O_NONBLOCK if needed
        fd_ = open(devicePath_.c_str(), O_RDWR);
    }

    if (fd_ < 0) {
        // Driver node does not exist or permission denied
        return false;
    }

    isConnected_ = true;
    return true;
}

void LinuxCharDevice::disconnect() {
    if (isConnected_ && fd_ >= 0) {
        close(fd_);
        fd_ = -1;
        isConnected_ = false;
    }
}

bool LinuxCharDevice::readEvent(wms_scan_event_t& outEvent, int timeoutMs) {
    if (!isConnected_ || fd_ < 0) return false;

    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN | POLLRDNORM;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeoutMs);
    if (ret <= 0) {
        return false; // Timeout or signal
    }

    if (pfd.revents & (POLLIN | POLLRDNORM)) {
        ssize_t bytesRead = read(fd_, &outEvent, sizeof(wms_scan_event_t));
        return (bytesRead == sizeof(wms_scan_event_t));
    }

    return false;
}

bool LinuxCharDevice::setBayLock(uint32_t bayId, bool lock) {
    if (!isConnected_ || fd_ < 0) return false;

    wms_bay_lock_req_t req;
    req.bay_id = bayId;
    req.lock_state = lock ? 1 : 0;

    return (ioctl(fd_, WMS_IOCTL_SET_BAY_LOCK, &req) == 0);
}

bool LinuxCharDevice::getBayLock(uint32_t bayId, bool& outLocked) {
    if (!isConnected_ || fd_ < 0) return false;

    wms_bay_lock_req_t req;
    req.bay_id = bayId;
    req.lock_state = 0;

    if (ioctl(fd_, WMS_IOCTL_GET_BAY_LOCK, &req) == 0) {
        outLocked = (req.lock_state != 0);
        return true;
    }
    return false;
}

bool LinuxCharDevice::getStatus(wms_device_status_t& outStatus) {
    if (!isConnected_ || fd_ < 0) return false;

    return (ioctl(fd_, WMS_IOCTL_GET_STATUS, &outStatus) == 0);
}

bool LinuxCharDevice::simulateScan(const wms_scan_event_t& event) {
    if (!isConnected_ || fd_ < 0) return false;

    return (ioctl(fd_, WMS_IOCTL_SIMULATE_SCAN, &event) == 0);
}

bool LinuxCharDevice::resetBuffer() {
    if (!isConnected_ || fd_ < 0) return false;

    return (ioctl(fd_, WMS_IOCTL_RESET_BUFFER) == 0);
}

} // namespace wms
