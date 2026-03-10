#pragma once
#include <vector>
#include <optional>
#include "coffee_order.h"

class MenuCache {
public:
    MenuCache() = default;

    void setVariants (std::vector<ProductVariant> v) { variants_  = std::move(v); }
    void setModifiers(std::vector<Modifier> m)        { modifiers_ = std::move(m); }

    const std::vector<ProductVariant>& getVariants()  const { return variants_;  }
    const std::vector<Modifier>&       getModifiers() const { return modifiers_; }

    bool variantsEmpty()  const { return variants_.empty();  }
    bool modifiersEmpty() const { return modifiers_.empty(); }

    void clear() { variants_.clear(); modifiers_.clear(); }

    std::optional<double> findVariantPrice(int variant_id) const {
        for (const auto& v : variants_) {
            if (v.variant_id == variant_id) return v.price;
        }
        return std::nullopt;
    }

    std::optional<double> findModifierPrice(int modifier_id) const {
        for (const auto& m : modifiers_) {
            if (m.modifier_id == modifier_id) return m.price_modifier;
        }
        return std::nullopt;
    }

private:
    std::vector<ProductVariant> variants_;
    std::vector<Modifier>       modifiers_;
};
