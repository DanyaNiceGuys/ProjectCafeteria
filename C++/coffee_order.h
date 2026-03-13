#pragma once
#include <string>
#include <vector>
#include "exceptions.h"

// ══════════════════════════════════════════════════════════════════════════════
// Содержит:
//   1. enum class OrderStatus         — статусы жизненного цикла заказа
//   2. constexpr statusToStringCE     — конвертация статуса в строку
//                                       вычисляется на этапе компиляции (constexpr)
//   3. statusToString / statusFromString — удобные обёртки
//   4. Структуры: Modifier, ProductVariant, OrderItem, CoffeeOrder
// ══════════════════════════════════════════════════════════════════════════════


// ── 1. Статусы заказа ─────────────────────────────────────────────────────────

enum class OrderStatus {
    NEW,        // только создан
    CONFIRMED,  // принят баристой
    PREPARING,  // готовится
    READY,      // готов, ждёт клиента
    COMPLETED,  // выдан
    CANCELLED   // отменён
};

// constexpr — функция вычисляется компилятором, а не в рантайме.
// statusToStringCE(OrderStatus::NEW) превращается в "new" до запуска программы.
constexpr const char* statusToStringCE(OrderStatus s) {
    switch (s) {
        case OrderStatus::NEW:       return "new";
        case OrderStatus::CONFIRMED: return "confirmed";
        case OrderStatus::PREPARING: return "preparing";
        case OrderStatus::READY:     return "ready";
        case OrderStatus::COMPLETED: return "completed";
        case OrderStatus::CANCELLED: return "cancelled";
    }
    return "new";
}

inline std::string statusToString(OrderStatus s) {
    return statusToStringCE(s);
}

inline OrderStatus statusFromString(const std::string& s) {
    if (s == "confirmed") return OrderStatus::CONFIRMED;
    if (s == "preparing") return OrderStatus::PREPARING;
    if (s == "ready")     return OrderStatus::READY;
    if (s == "completed") return OrderStatus::COMPLETED;
    if (s == "cancelled") return OrderStatus::CANCELLED;
    return OrderStatus::NEW; // неизвестный статус → NEW
}


// ── 2. Структуры данных ───────────────────────────────────────────────────────

// Модификатор напитка (тип молока, сироп, температура и т.д.)
struct Modifier {
    int         modifier_id;
    int         modifier_type_id;
    std::string type_name;           // например "Тип молока"
    std::string name;                // например "Овсяное молоко"
    double      price_modifier;      // добавка к цене в рублях
    bool        is_multiple_choice;  // можно ли выбрать несколько
};

// Конкретный вариант напитка из меню (Латте M горячий, Раф L холодный и т.д.)
struct ProductVariant {
    int         variant_id;
    int         product_id;
    std::string product_name;   // название напитка
    std::string category_name;  // категория (Кофе, Чай, ...)
    std::string size_name;      // размер (S, M, L)
    double      price;
    bool        is_hot_drink;
    bool        is_available;
};

// Одна позиция внутри заказа
struct OrderItem {
    int              order_item_id  = 0;
    int              variant_id;
    int              quantity       = 1;
    double           item_price;         // уже с учётом модификаторов и количества
    std::string      special_request;    // пожелания клиента по позиции
    std::vector<int> modifier_ids;       // выбранные модификаторы
};

// Заказ целиком
struct CoffeeOrder {
    int                    order_id     = 0;
    std::string            customer_phone;
    std::string            customer_name;
    double                 total_price  = 0.0;
    OrderStatus            status       = OrderStatus::NEW;
    std::string            special_instructions;  // общие пожелания к заказу
    std::string            created_at;
    std::vector<OrderItem> items;
};
