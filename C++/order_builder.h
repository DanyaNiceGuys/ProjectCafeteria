#pragma once
#include <string>
#include <vector>
#include "coffee_order.h"
#include "exceptions.h"

// Классы: PriceCalculator, OrderBuilder

class PriceCalculator {
public:
    PriceCalculator(std::vector<ProductVariant> variants,
                    std::vector<Modifier> modifiers)
            : variants_(std::move(variants)), modifiers_(std::move(modifiers)) {}

    // Цена позиции = (базовая цена + модификаторы) × количество
    double calculate(int variant_id,
                     const std::vector<int>& modifier_ids,
                     int quantity) const
    {
        double base = 0.0;
        for (const auto& v : variants_)
            if (v.variant_id == variant_id) { base = v.price; break; }

        double extras = 0.0;
        for (int mid : modifier_ids)
            for (const auto& m : modifiers_)
                if (m.modifier_id == mid) { extras += m.price_modifier; break; }

        return (base + extras) * quantity;
    }

    // Итоговая цена всего заказа = сумма всех позиций
    double calculateOrder(const CoffeeOrder& order) const {
        double total = 0.0;
        for (const auto& item : order.items)
            total += calculate(item.variant_id, item.modifier_ids, item.quantity);
        return total;
    }

private:
    std::vector<ProductVariant> variants_;
    std::vector<Modifier> modifiers_;
};

class OrderBuilder {
public:
    OrderBuilder() = default;

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

    OrderBuilder& addItem(int variant_id, int quantity,
                          const std::vector<int>& modifier_ids,
                          double item_price,
                          const std::string& special_request = "") {
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

    CoffeeOrder build() {
        if (order_.customer_name.empty())
            throw OrderValidationException("Имя клиента обязательно");
        if (order_.customer_phone.empty())
            throw OrderValidationException("Телефон клиента обязателен");
        if (order_.items.empty())
            throw OrderValidationException("Заказ должен содержать хотя бы одну позицию");
        order_.status = OrderStatus::NEW;
        return std::move(order_);
    }

    void reset() {
        order_ = CoffeeOrder{};
    }

private:
    CoffeeOrder order_;
};