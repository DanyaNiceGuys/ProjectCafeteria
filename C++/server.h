#pragma once
#include "httplib.h"
#include <nlohmann/json.hpp>
#include "database.h"
#include "menu_cache.h"
#include "order_builder.h"
#include "order_result.h"
#include <string>
#include <fstream>

using json = nlohmann::json;

// HTTP сервер кофейни.
//
// RequestHandler — обрабатывает входящие запросы.
//   Каждый метод handle* = один маршрут API.
//   Получает Database и MenuCache по ссылке (не владеет ими).
//   Содержит OrderLogger — демонстрирует shared_ptr на практике:
//   при создании заказа и сервер, и логгер держат shared_ptr на один объект.
//
// CafeteriaServer — оборачивает httplib::Server.
//   Регистрирует все маршруты и запускает сервер.



class RequestHandler {
public:
    RequestHandler(Database& db, MenuCache& cache)
        : db_(db), cache_(cache) {}

    // CORS заголовки — разрешаем запросы с любого домена (нужно для браузера)
    static void setCors(httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    }

    // GET /api/menu — список напитков, сгруппированных по категориям
    void handleMenu(const httplib::Request&, httplib::Response& res) {
        setCors(res);
        try {
            // Загружаем в кэш только при первом запросе
            if (cache_.variantsEmpty()) {
                cache_.setVariants(db_.getMenu());
            }
            json byCategory = json::object();
            for (const auto& v : cache_.getVariants()) {
                byCategory[v.category_name].push_back(variantToJson(v));
            }
            res.set_content(byCategory.dump(), "application/json");
        } catch (const DatabaseException& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    }

    // GET /api/modifiers — список модификаторов, сгруппированных по типу
    void handleModifiers(const httplib::Request&, httplib::Response& res) {
        setCors(res);
        try {
            if (cache_.modifiersEmpty()) {
                cache_.setModifiers(db_.getModifiers());
            }
            json byType = json::object();
            for (const auto& m : cache_.getModifiers()) {
                byType[m.type_name].push_back(modifierToJson(m));
            }
            res.set_content(byType.dump(), "application/json");
        } catch (const DatabaseException& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    }

    // POST /api/orders — создать новый заказ
    // Демонстрирует сразу несколько паттернов:
    //   OrderBuilder    — пошаговая сборка заказа
    //   PriceCalculator — подсчёт цены
    //   createOrderSafe — возвращает variant<int, string>
    //   std::visit      — обработка обоих случаев variant
    //   OrderLogger     — логирование через shared_ptr
    void handleCreateOrder(const httplib::Request& req, httplib::Response& res) {
        setCors(res);
        try {
            auto body = json::parse(req.body);

            if (cache_.variantsEmpty())  cache_.setVariants(db_.getMenu());
            if (cache_.modifiersEmpty()) cache_.setModifiers(db_.getModifiers());

            PriceCalculator calc(cache_.getVariants(), cache_.getModifiers());
            OrderBuilder    builder;

            builder.setCustomerName(body.at("customer_name").get<std::string>())
                   .setCustomerPhone(body.at("customer_phone").get<std::string>())
                   .setSpecialInstructions(body.value("special_instructions", ""));

            for (const auto& itemJson : body.at("items")) {
                int              variant_id = itemJson.at("variant_id").get<int>();
                int              quantity   = itemJson.value("quantity", 1);
                std::vector<int> mod_ids    = itemJson.value("modifier_ids", std::vector<int>{});
                std::string      special    = itemJson.value("special_request", "");
                double           price      = calc.calculate(variant_id, mod_ids, quantity);
                builder.addItem(variant_id, quantity, mod_ids, price, special);
            }

            // build() бросает OrderValidationException если данные некорректны
            CoffeeOrder order = builder.build();

            // createOrderSafe возвращает variant<int, string>
            // std::visit вызывает нужную лямбду в зависимости от типа внутри
            OrderResult result = db_.createOrderSafe(order);
            std::visit([&](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, int>) {
                    // val — это order_id (успех)
                    // Логируем через shared_ptr: и сервер, и логгер владеют заказом
                    logger_.log(makeSharedOrder(order));
                    res.status = 201;
                    res.set_content(json{
                        {"order_id",    val},
                        {"total_price", order.total_price},
                        {"status",      "new"}
                    }.dump(), "application/json");
                } else {
                    // val — это строка с ошибкой
                    res.status = 500;
                    res.set_content(json{{"error", val}}.dump(), "application/json");
                }
            }, result);

        } catch (const OrderValidationException& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");

        } catch (const DatabaseException& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } catch (const json::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("JSON: ") + e.what()}}.dump(),
                            "application/json");
        }
    }

    // GET /api/orders — список последних заказов
    void handleGetOrders(const httplib::Request&, httplib::Response& res) {
        setCors(res);
        try {
            auto orders = db_.getOrders();
            json arr = json::array();
            for (const auto& o : orders) arr.push_back(orderToJson(o));
            res.set_content(arr.dump(), "application/json");
        } catch (const DatabaseException& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    }

    // PATCH /api/orders/:id/status — изменить статус заказа
    void handleUpdateStatus(const httplib::Request& req, httplib::Response& res) {
        setCors(res);
        try {
            int id        = std::stoi(req.matches[1]);
            auto body     = json::parse(req.body);
            std::string s = body.at("status").get<std::string>();
            bool ok       = db_.updateStatus(id, statusFromString(s));
            res.set_content(json{{"ok", ok}}.dump(), "application/json");
        } catch (const json::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("JSON: ") + e.what()}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    }

private:
    Database&    db_;
    MenuCache&   cache_;
    OrderLogger  logger_; // логирует каждый созданный заказ через shared_ptr

    static json variantToJson(const ProductVariant& v) {
        return {
            {"variant_id",    v.variant_id},
            {"product_id",    v.product_id},
            {"product_name",  v.product_name},
            {"category_name", v.category_name},
            {"size_name",     v.size_name},
            {"price",         v.price},
            {"is_hot_drink",  v.is_hot_drink},
        };
    }

    static json modifierToJson(const Modifier& m) {
        return {
            {"modifier_id",        m.modifier_id},
            {"modifier_type_id",   m.modifier_type_id},
            {"type_name",          m.type_name},
            {"name",               m.name},
            {"price_modifier",     m.price_modifier},
            {"is_multiple_choice", m.is_multiple_choice},
        };
    }

    static json orderToJson(const CoffeeOrder& o) {
        json j = {
            {"order_id",             o.order_id},
            {"customer_name",        o.customer_name},
            {"customer_phone",       o.customer_phone},
            {"total_price",          o.total_price},
            {"status",               statusToString(o.status)},
            {"special_instructions", o.special_instructions},
            {"created_at",           o.created_at},
        };
        json items = json::array();
        for (const auto& item : o.items) {
            items.push_back({
                {"description", item.special_request},
                {"quantity",    item.quantity},
                {"item_price",  item.item_price},
            });
        }
        j["items"] = items;
        return j;
    }
};


// ── CafeteriaServer ───────────────────────────────────────────────────────────

class CafeteriaServer {
public:
    CafeteriaServer(Database& db, MenuCache& cache, const std::string& htmlPath)
        : handler_(db, cache), htmlPath_(htmlPath) {}

    void run(const std::string& host, int port) {
        registerRoutes();
        std::cout << "Сервер запущен: http://" << host << ":" << port << "\n";
        svr_.listen(host.c_str(), port);
    }

private:
    httplib::Server svr_;
    RequestHandler  handler_;
    std::string     htmlPath_;

    void registerRoutes() {
        svr_.Options(".*", [](const httplib::Request&, httplib::Response& res) {
            RequestHandler::setCors(res);
            res.status = 204;
        });

        // Главная страница — отдаём index.html
        svr_.Get("/", [this](const httplib::Request&, httplib::Response& res) {
            RequestHandler::setCors(res);
            std::ifstream file(htmlPath_);
            if (!file.is_open()) {
                res.status = 404;
                res.set_content("index.html not found", "text/plain");
                return;
            }
            std::string content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            res.set_content(content, "text/html; charset=utf-8");
        });

        svr_.Get ("/api/menu",      [this](auto& q, auto& r){ handler_.handleMenu(q,r); });
        svr_.Get ("/api/modifiers", [this](auto& q, auto& r){ handler_.handleModifiers(q,r); });
        svr_.Post("/api/orders",    [this](auto& q, auto& r){ handler_.handleCreateOrder(q,r); });
        svr_.Get ("/api/orders",    [this](auto& q, auto& r){ handler_.handleGetOrders(q,r); });

        svr_.Patch(R"(/api/orders/(\d+)/status)",
            [this](auto& q, auto& r){ handler_.handleUpdateStatus(q,r); });
    }
};
