#include "ipc/SignalDispatcher.hpp"
#include <iostream>
#include <cstring>

namespace wms {

std::atomic<bool> SignalDispatcher::shutdownRequested_{false};
std::atomic<bool> SignalDispatcher::reloadRequested_{false};

void SignalDispatcher::handleSignal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        shutdownRequested_.store(true);
    } else if (signum == SIGHUP) {
        reloadRequested_.store(true);
    }
}

void SignalDispatcher::initialize() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalDispatcher::handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
}

} // namespace wms
