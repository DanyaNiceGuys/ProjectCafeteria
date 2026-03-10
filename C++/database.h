#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include "coffee_order.h"
#include "menu_cache.h"
#include "order_result.h"

class Database {
public:
    explicit Database(const std::string& conninfo);
    ~Database() = default;
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&&) = default;
    Database& operator=(Database&&) = default;
    void connect();

    std::vector<ProductVariant> getMenu();
    std::vector<Modifier>       getModifiers();

    int createOrder (const CoffeeOrder& order);
    OrderResult createOrderSafe(const CoffeeOrder& order);
    std::vector<CoffeeOrder> getOrders();
    bool updateStatus(int order_id, OrderStatus status);

    // Найти по телефону или создать нового. Возвращает ID или nullopt при ошибке.
    std::optional<int> findOrCreateUser(const std::string& phone,
                                        const std::string& name);

private:
    std::string conninfo_;
    std::unique_ptr<pqxx::connection> conn_;
};
