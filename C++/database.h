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

// ══════════════════════════════════════════════════════════════════════════════
// ДАНЯ — database.h
// Класс Database — всё взаимодействие с PostgreSQL.
//
// Ключевые решения:
//
// unique_ptr<pqxx::connection> conn_  →  RAII
//   Соединение хранится в умном указателе. Когда объект Database уничтожается
//   (выходит из области видимости или вылетает исключение), соединение
//   закрывается автоматически. delete писать не нужно.
//
// Database(const Database&) = delete  →  запрет копирования
//   Соединение с БД — уникальный ресурс. Если бы копирование было разрешено,
//   два объекта могли бы работать с одним соединением одновременно → гонка данных.
//
// std::optional<int> findOrCreateUser(...)
//   Возвращает ID или nullopt при ошибке. Лучше чем возвращать -1 —
//   вызывающий код явно обязан проверить has_value().
//
// OrderResult createOrderSafe(...)
//   Версия без исключений: возвращает variant<int, string>.
//   Используется в сервере через std::visit.
// ══════════════════════════════════════════════════════════════════════════════

class Database {
public:
    explicit Database(const std::string& conninfo);

    // Деструктор = default: unique_ptr сам закроет соединение (RAII)
    ~Database() = default;

    // Соединение — уникальный ресурс, копирование запрещено
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // Перемещение разрешено (передать владение соединением)
    Database(Database&&)            = default;
    Database& operator=(Database&&) = default;

    // Открыть соединение. Бросает ConnectionException при неудаче.
    void connect();

    // ── Меню ──────────────────────────────────────────────────────────────────
    std::vector<ProductVariant> getMenu();
    std::vector<Modifier>       getModifiers();

    // ── Заказы ────────────────────────────────────────────────────────────────
    int         createOrder    (const CoffeeOrder& order); // бросает исключение
    OrderResult createOrderSafe(const CoffeeOrder& order); // возвращает variant
    std::vector<CoffeeOrder> getOrders();
    bool updateStatus(int order_id, OrderStatus status);

    // ── Пользователи ──────────────────────────────────────────────────────────
    // Найти по телефону или создать нового. Возвращает ID или nullopt при ошибке.
    std::optional<int> findOrCreateUser(const std::string& phone,
                                        const std::string& name);

private:
    std::string conninfo_;

    // unique_ptr — единственный владелец соединения.
    // При уничтожении Database соединение закроется автоматически.
    std::unique_ptr<pqxx::connection> conn_;
};
