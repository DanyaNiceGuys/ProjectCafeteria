#pragma once
#include <variant>
#include <memory>
#include <string>
#include <iostream>
#include "coffee_order.h"

using OrderResult = std::variant<int, std::string>;

inline bool isSuccess (const OrderResult& r) { return std::holds_alternative<int>(r); }
inline int getOrderId(const OrderResult& r) { return std::get<int>(r); }
inline std::string getError (const OrderResult& r) { return std::get<std::string>(r); }

using SharedOrder = std::shared_ptr<CoffeeOrder>;

inline SharedOrder makeSharedOrder(CoffeeOrder order) {
    return std::make_shared<CoffeeOrder>(std::move(order));
}

class OrderLogger {
public:
    void log(SharedOrder order) {
        last_ = order; // счётчик ссылок +1, теперь два владельца
        std::cout << "[LOG] Заказ #" << order->order_id
                  << " от "          << order->customer_name
                  << " на сумму "    << order->total_price << " ₽\n";
    }

    SharedOrder getLast()  const { return last_; }
    long useCount() const { return last_ ? last_.use_count() : 0; }

private:
    SharedOrder last_; // shared_ptr — не единственный владелец
};
