#pragma once
#include <string>
#include <vector>
#include "coffee_order.h"

// Классы: PriceCalculator, OrderBuilder

class PriceCalculator {
public:
    PriceCalculator(std::vector<ProductVariant> variants,
                    std::vector<Modifier> modifiers);

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

