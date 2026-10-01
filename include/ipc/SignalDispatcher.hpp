#pragma once

#include <csignal>
#include <atomic>
#include <functional>

namespace wms {

class SignalDispatcher {
public:
    using SignalCallback = std::function<void(int)>;

    static void initialize();
    static bool isShutdownRequested() noexcept { return shutdownRequested_.load(); }
    static bool isReloadRequested() noexcept { return reloadRequested_.load(); }
    static void resetReload() noexcept { reloadRequested_.store(false); }
    static void requestShutdown() noexcept { shutdownRequested_.store(true); }

private:
    static void handleSignal(int signum);
    static std::atomic<bool> shutdownRequested_;
    static std::atomic<bool> reloadRequested_;
};

} // namespace wms
