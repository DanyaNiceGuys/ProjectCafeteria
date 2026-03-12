#include <gtest/gtest.h>
#include "coffee_order.h"
#include "exceptions.h"
#include "order_builder.h"
#include "order_result.h"
#include "menu_cache.h"

static std::vector<ProductVariant> makeTestVariants() {
    ProductVariant v;
    v.variant_id    = 1;
    v.product_id    = 1;
    v.product_name  = "Латте";
    v.category_name = "Кофе";
    v.size_name     = "M";
    v.price         = 220.0;
    v.is_hot_drink  = true;
    v.is_available  = true;
    return {v};
}

static std::vector<Modifier> makeTestModifiers() {
    Modifier m;
    m.modifier_id        = 1;
    m.modifier_type_id   = 1;
    m.type_name          = "Тип молока";
    m.name               = "Овсяное молоко";
    m.price_modifier     = 80.0;
    m.is_multiple_choice = false;
    return {m};
}

TEST(PriceCalculatorTest, BasePriceWithoutModifiers) {
    PriceCalculator calc(makeTestVariants(), makeTestModifiers());
    EXPECT_DOUBLE_EQ(calc.calculate(1, {}, 1), 220.0);
}

TEST(PriceCalculatorTest, PriceWithModifier) {
    PriceCalculator calc(makeTestVariants(), makeTestModifiers());
    EXPECT_DOUBLE_EQ(calc.calculate(1, {1}, 1), 300.0);
}

TEST(PriceCalculatorTest, PriceScalesWithQuantity) {
    PriceCalculator calc(makeTestVariants(), makeTestModifiers());
    EXPECT_DOUBLE_EQ(calc.calculate(1, {}, 3), 660.0);
}

TEST(PriceCalculatorTest, UnknownVariantGivesZero) {
    PriceCalculator calc(makeTestVariants(), makeTestModifiers());
    EXPECT_DOUBLE_EQ(calc.calculate(999, {}, 1), 0.0);
}

TEST(OrderBuilderTest, BuildsValidOrder) {
    PriceCalculator calc(makeTestVariants(), makeTestModifiers());
    double price = calc.calculate(1, {1}, 1);

    CoffeeOrder order = OrderBuilder{}
        .setCustomerName("Иван")
        .setCustomerPhone("+79990000000")
        .setSpecialInstructions("Без сахара")
        .addItem(1, 1, {1}, price)
        .build();

    EXPECT_EQ(order.customer_name,      "Иван");
    EXPECT_EQ(order.customer_phone,     "+79990000000");
    EXPECT_EQ(order.items.size(),       1u);
    EXPECT_DOUBLE_EQ(order.total_price, 300.0);
    EXPECT_EQ(order.status,             OrderStatus::NEW);
}

TEST(OrderBuilderTest, ThrowsIfNameEmpty) {
    EXPECT_THROW({
        OrderBuilder{}.setCustomerName("").setCustomerPhone("+7").addItem(1,1,{},220.0).build();
    }, OrderValidationException);
}

TEST(OrderBuilderTest, ThrowsIfNoItems) {
    EXPECT_THROW({
        OrderBuilder{}.setCustomerName("Иван").setCustomerPhone("+7").build();
    }, OrderValidationException);
}

TEST(OrderBuilderTest, ResetClearsState) {
    OrderBuilder builder;
    builder.setCustomerName("Иван").setCustomerPhone("+7").addItem(1, 1, {}, 220.0);
    builder.reset();
    EXPECT_THROW({ builder.build(); }, OrderValidationException);
}

TEST(ExceptionTest, HierarchyWorks) {
    EXPECT_THROW({ throw ConnectionException("test"); }, ConnectionException);
    EXPECT_THROW({ throw ConnectionException("test"); }, DatabaseException);
    EXPECT_THROW({ throw ConnectionException("test"); }, CafeteriaException);
}

TEST(StatusTest, ToStringCoversAllStatuses) {
    EXPECT_EQ(statusToString(OrderStatus::NEW),       "new");
    EXPECT_EQ(statusToString(OrderStatus::CONFIRMED), "confirmed");
    EXPECT_EQ(statusToString(OrderStatus::PREPARING), "preparing");
    EXPECT_EQ(statusToString(OrderStatus::READY),     "ready");
    EXPECT_EQ(statusToString(OrderStatus::COMPLETED), "completed");
    EXPECT_EQ(statusToString(OrderStatus::CANCELLED), "cancelled");
}

TEST(MenuCacheTest, StartsEmpty) {
    MenuCache cache;
    EXPECT_TRUE(cache.variantsEmpty());
    EXPECT_TRUE(cache.modifiersEmpty());
}

TEST(MenuCacheTest, SetAndRetrieve) {
    MenuCache cache;
    cache.setVariants(makeTestVariants());
    cache.setModifiers(makeTestModifiers());
    EXPECT_EQ(cache.getVariants().size(),  1u);
    EXPECT_EQ(cache.getModifiers().size(), 1u);
}

TEST(MenuCacheTest, FindVariantPriceReturnsOptional) {
    MenuCache cache;
    cache.setVariants(makeTestVariants());

    auto found = cache.findVariantPrice(1);
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(*found, 220.0);

    auto missing = cache.findVariantPrice(999);
    EXPECT_FALSE(missing.has_value());
}

TEST(MenuCacheTest, ClearResetsCache) {
    MenuCache cache;
    cache.setVariants(makeTestVariants());
    cache.clear();
    EXPECT_TRUE(cache.variantsEmpty());
}

TEST(OrderResultTest, IntVariantIsSuccess) {
    OrderResult result = 42;
    EXPECT_TRUE(isSuccess(result));
    EXPECT_EQ(getOrderId(result), 42);
}

TEST(OrderResultTest, StringVariantIsError) {
    OrderResult result = std::string("Ошибка БД");
    EXPECT_FALSE(isSuccess(result));
    EXPECT_EQ(getError(result), "Ошибка БД");
}

TEST(OrderResultTest, VisitCallsCorrectBranch) {
    OrderResult result = 7;
    bool successCalled = false;
    std::visit([&](auto&& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) successCalled = true;
    }, result);
    EXPECT_TRUE(successCalled);
}

TEST(SharedPtrTest, SharedOwnership) {
    CoffeeOrder order;
    order.order_id      = 1;
    order.customer_name = "Маша";
    order.total_price   = 220.0;

    SharedOrder shared = makeSharedOrder(std::move(order));
    EXPECT_EQ(shared.use_count(), 1);

    OrderLogger logger;
    logger.log(shared);

    EXPECT_EQ(shared.use_count(), 2);
    EXPECT_EQ(logger.getLast()->customer_name, "Маша");
}
