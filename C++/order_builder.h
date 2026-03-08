#pragma once
#include <string>
#include <vector>
#include <optional>
#include "coffee_order.h"

// ══════════════════════════════════════════════════════════════════════════════
// ЛЁША — order_builder.h
// Сборка заказа и подсчёт цены.
//
// Содержит два класса:
//
// 1. PriceCalculator
//    Считает итоговую цену позиции: базовая цена варианта + модификаторы.
//    Вынесен отдельно чтобы его можно было независимо тестировать.
//
// 2. OrderBuilder — паттерн Builder
//    Пошагово собирает объект CoffeeOrder.
//    Fluent interface: каждый метод возвращает *this для цепочки вызовов:
//      builder.setCustomerName("Иван")
//             .setCustomerPhone("+7999...")
//             .addItem(1, 1, {2}, 300.0)
//             .build();
//    Метод build() проверяет данные и бросает OrderValidationException
//    если что-то не заполнено.
// ══════════════════════════════════════════════════════════════════════════════


// ── PriceCalculator ───────────────────────────────────────────────────────────

class PriceCalculator {
public:
    // Получает ссылки — не копирует данные, просто смотрит в кэш
    PriceCalculator(const std::vector<ProductVariant>& variants,
                    const std::vector<Modifier>&       modifiers)
            : variants_(variants), modifiers_(modifiers) {}

    // Цена одной позиции = (базовая цена + сумма доплат за модификаторы) × количество
    double calculate(int variant_id,
                     const std::vector<int>& modifier_ids,
                     int quantity) const
    {
        // Ищем базовую цену варианта
        double base = 0.0;
        for (const auto& v : variants_) {
            if (v.variant_id == variant_id) { base = v.price; break; }
        }

        // Суммируем доплаты за каждый модификатор
        double extras = 0.0;
        for (int mid : modifier_ids) {
            for (const auto& m : modifiers_) {
                if (m.modifier_id == mid) { extras += m.price_modifier; break; }
            }
        }

        return (base + extras) * quantity;
    }

    // Итоговая цена всего заказа = сумма цен всех позиций
    double calculateOrder(const CoffeeOrder& order) const {
        double total = 0.0;
        for (const auto& item : order.items) {
            total += calculate(item.variant_id, item.modifier_ids, item.quantity);
        }
        return total;
    }

private:
    const std::vector<ProductVariant>& variants_;
    const std::vector<Modifier>&       modifiers_;
};


// ── OrderBuilder ──────────────────────────────────────────────────────────────

class OrderBuilder {
public:
    OrderBuilder() = default;

    // Каждый setter возвращает *this — это позволяет писать цепочку вызовов
    OrderBuilder& setCustomerName(const std::string& name) {
        order_.customer_name = name;
        return *this;
    }

    OrderBuilder& setCustomerPhone(const std::string& phone) {
        order_.customer_phone = phone;
        return *this;
    }

    OrderBuilder& setSpecialInstructions(const std::string& instructions) {
        order_.special_instructions = instructions;
        return *this;
    }

    // Добавить позицию в заказ и сразу прибавить к итоговой цене
    OrderBuilder& addItem(int variant_id,
                          int quantity,
                          const std::vector<int>& modifier_ids,
                          double item_price,
                          const std::string& special_request = "")
    {
        OrderItem item;
        item.variant_id      = variant_id;
        item.quantity        = quantity;
        item.modifier_ids    = modifier_ids;
        item.item_price      = item_price;
        item.special_request = special_request;
        order_.items.push_back(std::move(item));
        order_.total_price += item_price;
        return *this;
    }

    // Финальная сборка: проверяет данные и возвращает готовый CoffeeOrder.
    // Бросает OrderValidationException если обязательные поля не заполнены.
    CoffeeOrder build() {
        if (order_.customer_name.empty())
            throw OrderValidationException("Имя клиента обязательно");
        if (order_.customer_phone.empty())
            throw OrderValidationException("Телефон клиента обязателен");
        if (order_.items.empty())
            throw OrderValidationException("Заказ должен содержать хотя бы одну позицию");

        order_.status = OrderStatus::NEW;
        return std::move(order_); // перемещаем, не копируем
    }

    // Сбросить билдер для повторного использования
    void reset() { order_ = CoffeeOrder{}; }

private:
    CoffeeOrder order_; // накапливаем данные пока не вызван build()
};
