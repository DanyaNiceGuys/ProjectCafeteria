#include "database.h"
#include <iostream>


Database::Database(const std::string& conninfo)
    : conninfo_(conninfo) {}

// Деструктор не пишем вручную — unique_ptr<pqxx::connection> вызовет
// деструктор соединения сам. Это RAII: ресурс живёт ровно столько,
// сколько живёт объект-владелец.

void Database::connect() {
    conn_ = std::make_unique<pqxx::connection>(conninfo_);
    if (!conn_->is_open()) {
        throw ConnectionException(conninfo_);
    }
    std::cout << "[DB] Подключено к: " << conn_->dbname() << "\n";
}


// Один запрос с JOIN-ами — эффективнее нескольких отдельных запросов.

std::vector<ProductVariant> Database::getMenu() {
    try {
        pqxx::work tx(*conn_);
        auto res = tx.exec(R"SQL(
            SELECT
                pv.variant_id,
                pv.product_id,
                p.name          AS product_name,
                c.name          AS category_name,
                s.name          AS size_name,
                pv.price,
                pv.is_hot_drink,
                pv.is_available
            FROM product_variants pv
            JOIN products   p ON p.product_id  = pv.product_id
            JOIN categories c ON c.category_id = p.category_id
            JOIN sizes       s ON s.size_id     = pv.size_id
            WHERE pv.is_available = TRUE
              AND pv.price IS NOT NULL
              AND p.stock_quantity > 0
            ORDER BY c.sort_order, p.name, s.display_order, pv.is_hot_drink DESC
        )SQL");

        std::vector<ProductVariant> variants;
        variants.reserve(res.size());
        for (const auto& row : res) {
            ProductVariant v;
            v.variant_id    = row["variant_id"].as<int>();
            v.product_id    = row["product_id"].as<int>();
            v.product_name  = row["product_name"].as<std::string>();
            v.category_name = row["category_name"].as<std::string>();
            v.size_name     = row["size_name"].as<std::string>();
            v.price         = row["price"].as<double>();
            v.is_hot_drink  = row["is_hot_drink"].as<bool>();
            v.is_available  = row["is_available"].as<bool>();
            variants.push_back(std::move(v));
        }
        return variants;
    } catch (const DatabaseException&) {
        throw; // перебрасываем — не оборачиваем дважды
    } catch (const std::exception& e) {
        throw DatabaseException(std::string("getMenu(): ") + e.what());
    }
}

std::vector<Modifier> Database::getModifiers() {
    try {
        pqxx::work tx(*conn_);
        auto res = tx.exec(R"SQL(
            SELECT
                m.modifier_id,
                m.modifier_type_id,
                mt.name             AS type_name,
                m.name,
                m.price_modifier,
                mt.is_multiple_choice
            FROM modifiers m
            JOIN modifier_types mt USING (modifier_type_id)
            WHERE m.is_available = TRUE
            ORDER BY mt.modifier_type_id, m.modifier_id
        )SQL");

        std::vector<Modifier> mods;
        mods.reserve(res.size());
        for (const auto& row : res) {
            Modifier mod;
            mod.modifier_id        = row["modifier_id"].as<int>();
            mod.modifier_type_id   = row["modifier_type_id"].as<int>();
            mod.type_name          = row["type_name"].as<std::string>();
            mod.name               = row["name"].as<std::string>();
            mod.price_modifier     = row["price_modifier"].as<double>();
            mod.is_multiple_choice = row["is_multiple_choice"].as<bool>();
            mods.push_back(std::move(mod));
        }
        return mods;
    } catch (const DatabaseException&) {
        throw;
    } catch (const std::exception& e) {
        throw DatabaseException(std::string("getModifiers(): ") + e.what());
    }
}

// Ищет пользователя по телефону. Если не нашёл — создаёт нового.
// Возвращает std::optional<int>: ID при успехе или nullopt при ошибке.

std::optional<int> Database::findOrCreateUser(const std::string& phone,
                                               const std::string& name) {
    try {
        pqxx::work tx(*conn_);

        auto res = tx.exec_params(
            "SELECT user_id FROM users WHERE phone_number = $1", phone);

        int user_id;
        if (!res.empty()) {
            user_id = res[0][0].as<int>(); // нашли существующего
        } else {
            auto ins = tx.exec_params(
                "INSERT INTO users (phone_number, full_name) "
                "VALUES ($1, $2) RETURNING user_id",
                phone, name);
            user_id = ins[0][0].as<int>(); // создали нового
        }

        tx.commit();
        return user_id;      // optional с значением — всё хорошо
    } catch (const std::exception& e) {
        std::cerr << "[DB] findOrCreateUser() error: " << e.what() << "\n";
        return std::nullopt; // optional пустой — что-то пошло не так
    }
}

// Создаёт заказ в БД в одной транзакции (всё или ничего):
//   1. Найти или создать пользователя
//   2. Вставить запись в orders
//   3. Для каждой позиции вставить order_items + order_item_modifiers

int Database::createOrder(const CoffeeOrder& order) {
    if (order.customer_name.empty() || order.customer_phone.empty())
        throw OrderValidationException("Имя и телефон обязательны");
    if (order.items.empty())
        throw OrderValidationException("Заказ должен содержать хотя бы одну позицию");

    try {
        pqxx::work tx(*conn_);

        // Шаг 1: найти или создать пользователя
        int user_id = -1;
        {
            auto res = tx.exec_params(
                "SELECT user_id FROM users WHERE phone_number = $1",
                order.customer_phone);
            if (!res.empty()) {
                user_id = res[0][0].as<int>();
            } else {
                auto ins = tx.exec_params(
                    "INSERT INTO users (phone_number, full_name) "
                    "VALUES ($1, $2) RETURNING user_id",
                    order.customer_phone, order.customer_name);
                user_id = ins[0][0].as<int>();
            }
        }

        // Шаг 2: создать заказ
        auto res = tx.exec_params(R"SQL(
            INSERT INTO orders
                (user_id, customer_phone, customer_name,
                 status, total_price, special_instructions)
            VALUES ($1, $2, $3, $4, $5, $6)
            RETURNING order_id
        )SQL",
            user_id, order.customer_phone, order.customer_name,
            statusToString(order.status), order.total_price,
            order.special_instructions);

        int order_id = res[0][0].as<int>();

        // Шаг 3: вставить позиции и модификаторы к ним
        for (const auto& item : order.items) {
            auto iRes = tx.exec_params(R"SQL(
                INSERT INTO order_items
                    (order_id, variant_id, quantity, item_price, special_request)
                VALUES ($1, $2, $3, $4, $5)
                RETURNING order_item_id
            )SQL",
                order_id, item.variant_id, item.quantity,
                item.item_price, item.special_request);

            int order_item_id = iRes[0][0].as<int>();

            for (int mod_id : item.modifier_ids) {
                tx.exec_params(
                    "INSERT INTO order_item_modifiers "
                    "(order_item_id, modifier_id) VALUES ($1, $2)",
                    order_item_id, mod_id);
            }
        }

        tx.commit(); // фиксируем транзакцию — всё или ничего
        std::cout << "[DB] Создан заказ #" << order_id << "\n";
        return order_id;
    } catch (const CafeteriaException&) {
        throw;
    } catch (const std::exception& e) {
        throw DatabaseException(std::string("createOrder(): ") + e.what());
    }
}

// Обёртка над createOrder без исключений.
// Возвращает std::variant<int, string> — сервер обрабатывает через std::visit.

OrderResult Database::createOrderSafe(const CoffeeOrder& order) {
    try {
        return createOrder(order);          // variant с int внутри
    } catch (const CafeteriaException& e) {
        return std::string(e.what());       // variant со string внутри
    } catch (const std::exception& e) {
        return std::string(e.what());
    }
}

// Загружает последние 200 заказов с позициями.
// Используем ДВА запроса вместо N+1:
//   Запрос 1 — все заказы
//   Запрос 2 — все позиции для этих заказов одним JOIN-ом (с GROUP BY для модификаторов)
// Затем раскладываем позиции по заказам через unordered_map.

std::vector<CoffeeOrder> Database::getOrders() {
    try {
        pqxx::work tx(*conn_);

        // Запрос 1: загрузить заказы
        auto res = tx.exec(R"SQL(
            SELECT
                o.order_id,
                o.customer_phone,
                o.customer_name,
                o.total_price,
                o.status,
                o.special_instructions,
                to_char(o.created_at, 'DD.MM HH24:MI') AS created_at
            FROM orders o
            ORDER BY o.order_id DESC
            LIMIT 200
        )SQL");

        std::vector<CoffeeOrder> orders;
        orders.reserve(res.size());
        for (const auto& row : res) {
            CoffeeOrder o;
            o.order_id             = row["order_id"].as<int>();
            o.customer_phone       = row["customer_phone"].as<std::string>();
            o.customer_name        = row["customer_name"].c_str();
            o.total_price          = row["total_price"].as<double>();
            o.status               = statusFromString(row["status"].as<std::string>());
            o.special_instructions = row["special_instructions"].c_str();
            o.created_at           = row["created_at"].as<std::string>();
            orders.push_back(std::move(o));
        }

        if (orders.empty()) return orders;

        // Запрос 2: все позиции одним запросом (избегаем N+1)
        // string_agg объединяет модификаторы в одну строку через запятую

        auto itemRes = tx.exec(R"SQL(
            SELECT
                oi.order_id,
                oi.order_item_id,
                oi.variant_id,
                oi.quantity,
                oi.item_price,
                p.name          AS product_name,
                s.name          AS size_name,
                pv.is_hot_drink,
                COALESCE(
                    string_agg(m.name, ', ' ORDER BY m.modifier_id), ''
                )               AS modifiers_list
            FROM order_items oi
            JOIN product_variants pv ON pv.variant_id = oi.variant_id
            JOIN products         p  ON p.product_id  = pv.product_id
            JOIN sizes            s  ON s.size_id      = pv.size_id
            LEFT JOIN order_item_modifiers oim ON oim.order_item_id = oi.order_item_id
            LEFT JOIN modifiers            m   ON m.modifier_id     = oim.modifier_id
            WHERE oi.order_id IN (
                SELECT o.order_id FROM orders o ORDER BY o.order_id DESC LIMIT 200
            )
            GROUP BY oi.order_id, oi.order_item_id, oi.variant_id,
                     oi.quantity, oi.item_price, p.name, s.name, pv.is_hot_drink
        )SQL");

        // Раскладываем позиции по заказам через unordered_map для O(1) поиска
        std::unordered_map<int, std::vector<OrderItem>> itemsByOrder;
        for (const auto& row : itemRes) {
            OrderItem item;
            item.order_item_id = row["order_item_id"].as<int>();
            item.variant_id    = row["variant_id"].as<int>();
            item.quantity      = row["quantity"].as<int>();
            item.item_price    = row["item_price"].as<double>();

            std::string label = row["product_name"].as<std::string>()
                              + " " + row["size_name"].as<std::string>()
                              + (row["is_hot_drink"].as<bool>() ? " (гор)" : " (хол)");
            std::string mods  = row["modifiers_list"].as<std::string>();
            item.special_request = mods.empty() ? label : label + ", " + mods;

            itemsByOrder[row["order_id"].as<int>()].push_back(std::move(item));
        }

        // Прикрепляем позиции к своим заказам
        for (auto& order : orders) {
            auto it = itemsByOrder.find(order.order_id);
            if (it != itemsByOrder.end()) {
                order.items = std::move(it->second);
            }
        }

        return orders;
    } catch (const DatabaseException&) {
        throw;
    } catch (const std::exception& e) {
        throw DatabaseException(std::string("getOrders(): ") + e.what());
    }
}

bool Database::updateStatus(int order_id, OrderStatus status) {
    try {
        pqxx::work tx(*conn_);
        tx.exec_params(
            "UPDATE orders SET status = $1 WHERE order_id = $2",
            statusToString(status), order_id);
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DB] updateStatus() error: " << e.what() << "\n";
        return false;
    }
}
