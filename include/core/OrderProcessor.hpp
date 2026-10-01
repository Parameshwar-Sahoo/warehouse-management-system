#pragma once

#include "Item.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

namespace wms {

enum class OrderStatus {
    PENDING,
    PROCESSING,
    FULFILLED,
    FAILED
};

struct OrderItem {
    std::string sku;
    int quantity;
    int fulfilledQuantity{0};
};

struct Order {
    std::string orderId;
    std::string destination;
    std::vector<OrderItem> items;
    OrderStatus status{OrderStatus::PENDING};
    uint64_t createdAt{0};
    uint64_t completedAt{0};

    Order() = default;
    Order(std::string id, std::string dest, std::vector<OrderItem> orderItems)
        : orderId(std::move(id)), destination(std::move(dest)), items(std::move(orderItems)),
          status(OrderStatus::PENDING),
          createdAt(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count()) {}

    static std::string statusToString(OrderStatus s) {
        switch (s) {
        case OrderStatus::PENDING: return "Pending";
        case OrderStatus::PROCESSING: return "Processing";
        case OrderStatus::FULFILLED: return "Fulfilled";
        case OrderStatus::FAILED: return "Failed";
        default: return "Unknown";
        }
    }
};

class InventoryManager; // Forward declaration

class OrderProcessor {
public:
    explicit OrderProcessor(InventoryManager& inventory);

    bool submitOrder(const Order& order);
    bool processOrder(const std::string& orderId);
    std::vector<Order> getPendingOrders() const;
    std::vector<Order> getAllOrders() const;
    size_t getFulfilledCount() const noexcept { return fulfilledCount_; }

private:
    InventoryManager& inventory_;
    std::vector<Order> orders_;
    mutable std::mutex mutex_;
    size_t fulfilledCount_{0};
};

} // namespace wms
