#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include "coffee_order.h"
#include "database.h"
#include "menu_cache.h"
#include "order_builder.h"
#include "order_result.h"


class ConsoleUI {
public:
    ConsoleUI(Database& db, MenuCache& cache)
            : db_(db), cache_(cache) {}

    void run() {
        std::cout << "\n╔══════════════════════════════╗\n";
        std::cout <<   "║   Кофейня Matreshka ☕       ║\n";
        std::cout <<   "╚══════════════════════════════╝\n";

        while (true) {
            printMainMenu();
            int choice = readInt("Выберите действие: ");

            switch (choice) {
                case 1: showMenu();     break;
                case 2: createOrder();  break;
                case 3: showOrders();   break;
                case 4: updateStatus(); break;
                case 0:
                    std::cout << "До свидания!\n";
                    return;
                default:
                    std::cout << "Неверный выбор, попробуйте снова.\n";
            }
        }
    }

private:
    Database&  db_;
    MenuCache& cache_;

    void printMainMenu() const {
        std::cout << "\n┌─────────────────────────────┐\n";
        std::cout <<   "│  1. Показать меню            │\n";
        std::cout <<   "│  2. Создать заказ            │\n";
        std::cout <<   "│  3. Показать все заказы      │\n";
        std::cout <<   "│  4. Обновить статус заказа   │\n";
        std::cout <<   "│  0. Выход                    │\n";
        std::cout <<   "└─────────────────────────────┘\n";
    }

    void showMenu() {
        if (cache_.variantsEmpty())
            cache_.setVariants(db_.getMenu());
        if (cache_.modifiersEmpty())
            cache_.setModifiers(db_.getModifiers());

        std::cout << "\n═══ МЕНЮ ═══\n";
        std::string currentCategory;
        for (const auto& v : cache_.getVariants()) {
            if (v.category_name != currentCategory) {
                currentCategory = v.category_name;
                std::cout << "\n[ " << currentCategory << " ]\n";
            }
            std::cout << "  [" << v.variant_id << "] "
                      << v.product_name << " " << v.size_name
                      << (v.is_hot_drink ? " (гор)" : " (хол)")
                      << " — " << v.price << " ₽\n";
        }

        std::cout << "\n═══ МОДИФИКАТОРЫ ═══\n";
        std::string currentType;
        for (const auto& m : cache_.getModifiers()) {
            if (m.type_name != currentType) {
                currentType = m.type_name;
                std::cout << "\n[ " << currentType << " ]\n";
            }
            std::cout << "  [" << m.modifier_id << "] "
                      << m.name << " +" << m.price_modifier << " ₽\n";
        }
    }

    void createOrder() {
        if (cache_.variantsEmpty())  cache_.setVariants(db_.getMenu());
        if (cache_.modifiersEmpty()) cache_.setModifiers(db_.getModifiers());

        std::cout << "\n═══ НОВЫЙ ЗАКАЗ ═══\n";

        std::string name  = readString("Ваше имя: ");
        std::string phone = readString("Телефон: ");
        std::string notes = readString("Пожелания (Enter — пропустить): ");

        PriceCalculator calc(cache_.getVariants(), cache_.getModifiers());
        OrderBuilder    builder;
        builder.setCustomerName(name)
                .setCustomerPhone(phone)
                .setSpecialInstructions(notes);

        while (true) {
            showMenu();
            int variantId = readInt("Введите ID напитка (0 - завершить): ");
            if (variantId == 0) break;

            int qty = readInt("Количество: ");

            std::cout << "Введите ID модификаторов через пробел (Enter - без): ";
            std::string line;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, line);

            std::vector<int> modIds;
            std::istringstream iss(line);
            int mid;
            while (iss >> mid) modIds.push_back(mid);

            double price = calc.calculate(variantId, modIds, qty);
            builder.addItem(variantId, qty, modIds, price);
            std::cout << "Добавлено. Цена позиции: " << price << " ₽\n";
        }

        try {
            CoffeeOrder order = builder.build();
            OrderResult result = db_.createOrderSafe(order);

            std::visit([&](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, int>) {
                std::cout << "✅ Заказ #" << val
                          << " создан! Итого: " << order.total_price << " ₽\n";
            } else {
                std::cout << "❌ Ошибка: " << val << "\n";
            }
            }, result);

        } catch (const OrderValidationException& e) {
            std::cout << "❌ " << e.what() << "\n";
        }
    }

    void showOrders() {
        auto orders = db_.getOrders();
        if (orders.empty()) {
            std::cout << "Заказов пока нет.\n";
            return;
        }

        std::cout << "\n═══ ЗАКАЗЫ ═══\n";
        for (const auto& o : orders) {
            std::cout << "\n#" << o.order_id
                      << " | " << o.customer_name
                      << " | " << o.customer_phone
                      << " | " << statusToString(o.status)
                      << " | " << o.total_price << " ₽"
                      << " | " << o.created_at << "\n";
            for (const auto& item : o.items) {
                std::cout << "   - " << item.special_request
                          << " x" << item.quantity
                          << " = " << item.item_price << " ₽\n";
            }
        }
    }

    void updateStatus() {
        showOrders();
        int id = readInt("\nID заказа: ");

        std::cout << "Статусы: new / confirmed / preparing / ready / completed / cancelled\n";
        std::string status = readString("Новый статус: ");

        bool ok = db_.updateStatus(id, statusFromString(status));
        std::cout << (ok ? "✅ Статус обновлён\n" : "❌ Ошибка обновления\n");
    }

    int readInt(const std::string& prompt) {
        int val;
        while (true) {
            std::cout << prompt;
            if (std::cin >> val) return val;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Введите число.\n";
        }
    }

    std::string readString(const std::string& prompt) {
        std::string val;
        std::cout << prompt;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, val);
        return val;
    }
};