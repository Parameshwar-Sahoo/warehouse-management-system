#include "core/OrderProcessor.hpp"
#include "core/InventoryManager.hpp"
#include <iostream>

namespace wms {

OrderProcessor::OrderProcessor(InventoryManager& inventory)
    : inventory_(inventory) {}

bool OrderProcessor::submitOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    orders_.push_back(order);
    return true;
}

bool OrderProcessor::processOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& order : orders_) {
        if (order.orderId == orderId) {
            if (order.status == OrderStatus::FULFILLED) return true;

            order.status = OrderStatus::PROCESSING;
            bool allFulfilled = true;

            for (auto& item : order.items) {
                int needed = item.quantity - item.fulfilledQuantity;
                if (needed > 0) {
                    std::vector<uint32_t> clearedBays;
                    if (inventory_.dispatchItem(item.sku, needed, clearedBays)) {
                        item.fulfilledQuantity += static_cast<int>(clearedBays.size());
                    } else {
                        allFulfilled = false;
                    }
                }
            }

            if (allFulfilled) {
                order.status = OrderStatus::FULFILLED;
                order.completedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
                fulfilledCount_++;
                return true;
            } else {
                order.status = OrderStatus::FAILED;
                return false;
            }
        }
    }
    return false;
}

std::vector<Order> OrderProcessor::getPendingOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Order> pending;
    for (const auto& ord : orders_) {
        if (ord.status == OrderStatus::PENDING) {
            pending.push_back(ord);
        }
    }
    return pending;
}

std::vector<Order> OrderProcessor::getAllOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_;
}

} // namespace wms
