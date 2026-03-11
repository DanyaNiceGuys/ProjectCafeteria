#pragma once
#include <string>
#include <vector>
#include "coffee_order.h"

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

private:
    std::vector<ProductVariant> variants_;
    std::vector<Modifier> modifiers_;
};

class OrderBuilder {
public:
    OrderBuilder() = default;

private:
    CoffeeOrder order_;
};
