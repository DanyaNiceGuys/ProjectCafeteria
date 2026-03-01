-- ============================================
-- ПОЛНЫЙ ФАЙЛ СОЗДАНИЯ БАЗЫ ДАННЫХ КОФЕЙНИ
-- ВЫПОЛНИТЬ ЦЕЛИКОМ В QUERY TOOL PGADMIN 
-- ============================================

-- ============================================
-- 1. УДАЛЕНИЕ СТАРЫХ ТАБЛИЦ (если существуют)
-- ============================================
DROP TABLE IF EXISTS cart_item_modifiers CASCADE;
DROP TABLE IF EXISTS cart_items CASCADE;
DROP TABLE IF EXISTS carts CASCADE;
DROP TABLE IF EXISTS order_item_modifiers CASCADE;
DROP TABLE IF EXISTS order_items CASCADE;
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS product_variants CASCADE;
DROP TABLE IF EXISTS modifiers CASCADE;
DROP TABLE IF EXISTS modifier_types CASCADE;
DROP TABLE IF EXISTS sizes CASCADE;
DROP TABLE IF EXISTS products CASCADE;
DROP TABLE IF EXISTS categories CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- ============================================
-- 2. СОЗДАНИЕ ТАБЛИЦ
-- ============================================

-- 2.1. Пользователи (вход через пуш-уведомления)
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    phone_number VARCHAR(20) UNIQUE NOT NULL,
    full_name VARCHAR(100),
    email VARCHAR(100),
    registered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2.2. Категории напитков
CREATE TABLE categories (
    category_id SERIAL PRIMARY KEY,
    name VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    sort_order INTEGER DEFAULT 0
);

-- 2.3. Размеры порций
CREATE TABLE sizes (
    size_id SERIAL PRIMARY KEY,
    name VARCHAR(10) UNIQUE NOT NULL,
    display_order INTEGER DEFAULT 0
);

-- 2.4. Продукты с остатками
CREATE TABLE products (
    product_id SERIAL PRIMARY KEY,
    category_id INTEGER REFERENCES categories(category_id) ON DELETE SET NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    image_url VARCHAR(255),
    stock_quantity INTEGER NOT NULL DEFAULT 0 CHECK (stock_quantity >= 0),
    CONSTRAINT fk_category FOREIGN KEY (category_id) REFERENCES categories(category_id)
);

-- 2.5. Варианты продукта (размер + температура + цена)
CREATE TABLE product_variants (
    variant_id SERIAL PRIMARY KEY,
    product_id INTEGER NOT NULL REFERENCES products(product_id) ON DELETE CASCADE,
    size_id INTEGER NOT NULL REFERENCES sizes(size_id),
    price DECIMAL(10, 2) CHECK (price >= 0),
    is_available BOOLEAN DEFAULT TRUE,
    is_hot_drink BOOLEAN DEFAULT TRUE,
    UNIQUE(product_id, size_id, is_hot_drink)
);

-- 2.6. Типы модификаторов (сиропы, молоко и т.д.)
CREATE TABLE modifier_types (
    modifier_type_id SERIAL PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE,
    description VARCHAR(255),
    is_required BOOLEAN DEFAULT FALSE,
    is_multiple_choice BOOLEAN DEFAULT FALSE
);

-- 2.7. Конкретные модификаторы
CREATE TABLE modifiers (
    modifier_id SERIAL PRIMARY KEY,
    modifier_type_id INTEGER NOT NULL REFERENCES modifier_types(modifier_type_id) ON DELETE CASCADE,
    name VARCHAR(50) NOT NULL,
    price_modifier DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
    is_available BOOLEAN DEFAULT TRUE
);

-- 2.8. Заказы (основная таблица)
CREATE TABLE orders (
    order_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id) ON DELETE SET NULL,
    customer_phone VARCHAR(20) NOT NULL,
    customer_name VARCHAR(100),
    status VARCHAR(20) NOT NULL DEFAULT 'new' CHECK (status IN ('new', 'confirmed', 'preparing', 'ready', 'completed', 'cancelled')),
    total_price DECIMAL(10, 2) NOT NULL CHECK (total_price >= 0),
    payment_method VARCHAR(20) CHECK (payment_method IN ('online', 'cash', 'card_on_pickup')),
    payment_status VARCHAR(20) DEFAULT 'pending' CHECK (payment_status IN ('pending', 'paid', 'refunded')),
    is_cash_entered BOOLEAN DEFAULT FALSE,
    special_instructions TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2.9. Позиции в заказе
CREATE TABLE order_items (
    order_item_id SERIAL PRIMARY KEY,
    order_id INTEGER NOT NULL REFERENCES orders(order_id) ON DELETE CASCADE,
    variant_id INTEGER NOT NULL REFERENCES product_variants(variant_id) ON DELETE RESTRICT,
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    item_price DECIMAL(10, 2) NOT NULL CHECK (item_price >= 0),
    special_request TEXT
);

-- 2.10. Модификаторы для позиций заказа
CREATE TABLE order_item_modifiers (
    id SERIAL PRIMARY KEY,
    order_item_id INTEGER NOT NULL REFERENCES order_items(order_item_id) ON DELETE CASCADE,
    modifier_id INTEGER NOT NULL REFERENCES modifiers(modifier_id) ON DELETE RESTRICT,
    UNIQUE(order_item_id, modifier_id)
);

-- 2.11. Корзины пользователей
CREATE TABLE carts (
    cart_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2.12. Элементы корзины
CREATE TABLE cart_items (
    cart_item_id SERIAL PRIMARY KEY,
    cart_id INTEGER NOT NULL REFERENCES carts(cart_id) ON DELETE CASCADE,
    variant_id INTEGER NOT NULL REFERENCES product_variants(variant_id) ON DELETE RESTRICT,
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2.13. Модификаторы для элементов корзины
CREATE TABLE cart_item_modifiers (
    id SERIAL PRIMARY KEY,
    cart_item_id INTEGER NOT NULL REFERENCES cart_items(cart_item_id) ON DELETE CASCADE,
    modifier_id INTEGER NOT NULL REFERENCES modifiers(modifier_id) ON DELETE RESTRICT,
    UNIQUE(cart_item_id, modifier_id)
);

-- ============================================
-- 3. ИНДЕКСЫ ДЛЯ УСКОРЕНИЯ ЗАПРОСОВ
-- ============================================
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_orders_created_at ON orders(created_at);
CREATE INDEX idx_orders_cash_entered ON orders(is_cash_entered) WHERE is_cash_entered = FALSE;
CREATE INDEX idx_order_items_order_id ON order_items(order_id);
CREATE INDEX idx_products_category ON products(category_id);
CREATE INDEX idx_products_stock ON products(stock_quantity) WHERE stock_quantity > 0;
CREATE INDEX idx_product_variants_product ON product_variants(product_id);
CREATE INDEX idx_product_variants_size ON product_variants(size_id);
CREATE INDEX idx_carts_user ON carts(user_id);
CREATE INDEX idx_cart_items_cart ON cart_items(cart_id);

-- ============================================
-- 4. ТРИГГЕРЫ И ФУНКЦИИ
-- ============================================

-- 4.1. Функция для обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- 4.2. Триггер для обновления времени в заказах
CREATE TRIGGER update_orders_updated_at 
    BEFORE UPDATE ON orders 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- 4.3. Триггер для обновления времени в корзинах
CREATE TRIGGER update_carts_updated_at 
    BEFORE UPDATE ON carts 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- 4.4. Функция проверки остатков при добавлении в корзину
CREATE OR REPLACE FUNCTION check_stock_before_cart()
RETURNS TRIGGER AS $$
DECLARE
    current_stock INTEGER;
BEGIN
    SELECT stock_quantity INTO current_stock
    FROM products p
    JOIN product_variants pv ON p.product_id = pv.product_id
    WHERE pv.variant_id = NEW.variant_id;
    
    IF current_stock < NEW.quantity THEN
        RAISE EXCEPTION 'Недостаточно товара в наличии. Доступно: %', current_stock;
    END IF;
    
    RETURN NEW;
END;
$$ language 'plpgsql';

-- 4.5. Триггер проверки остатков для корзины
CREATE TRIGGER check_cart_item_stock 
    BEFORE INSERT OR UPDATE ON cart_items 
    FOR EACH ROW EXECUTE FUNCTION check_stock_before_cart();

-- 4.6. Функция обновления остатков при подтверждении заказа
CREATE OR REPLACE FUNCTION update_stock_on_order()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.status = 'confirmed' AND OLD.status != 'confirmed' THEN
        UPDATE products p
        SET stock_quantity = stock_quantity - oi.quantity
        FROM order_items oi
        JOIN product_variants pv ON oi.variant_id = pv.variant_id
        WHERE oi.order_id = NEW.order_id 
          AND pv.product_id = p.product_id;
    END IF;
    
    RETURN NEW;
END;
$$ language 'plpgsql';

-- 4.7. Триггер обновления остатков
CREATE TRIGGER update_product_stock 
    AFTER UPDATE ON orders 
    FOR EACH ROW EXECUTE FUNCTION update_stock_on_order();

-- ============================================
-- 5. ЗАПОЛНЕНИЕ ДАННЫМИ (НАЧАЛО ТРАНЗАКЦИИ)
-- ============================================
BEGIN;

-- 5.1. Категории
INSERT INTO categories (name, sort_order) VALUES
('Кофе', 1),
('Чай', 2),
('Раф-кофе', 3);

-- 5.2. Размеры
INSERT INTO sizes (name, display_order) VALUES
('S', 1),
('M', 2),
('L', 3),
('50 мл', 0);

-- 5.3. Типы модификаторов
INSERT INTO modifier_types (name, description, is_multiple_choice) VALUES
('Температура', 'Горячий или холодный напиток', false),
('Сироп', 'Добавка сиропа', true),
('Дополнения', 'Дополнительные ингредиенты', true),
('Тип кофе', 'Выбор типа кофейных зерен', false),
('Тип молока', 'Выбор альтернативного молока', false);

-- 5.4. Модификаторы (все сиропы + дополнения)
INSERT INTO modifiers (modifier_type_id, name, price_modifier) VALUES
-- Температура
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Температура'), 'Горячий', 0.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Температура'), 'Холодный', 0.00),
-- Сиропы (полный список)
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Пряный манго', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Ежевика', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Можжевельник', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Банан', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Фисташка', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Амаретто', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Арахис', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Миндаль', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Лаванда', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Кокос', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Ваниль', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Мята', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Клубника', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Груша', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Имбирный пряник', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Соленая карамель', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Шоколадное печенье', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Ирландский кофе', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Попкорн', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Яблочный пирог', 30.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Сироп'), 'Карамельный', 30.00),
-- Дополнения
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Дополнения'), 'Дополнительный шот эспрессо', 50.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Дополнения'), 'Маршмеллоу', 50.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Тип кофе'), 'Кофе без кофеина', 70.00),
-- Альтернативное молоко
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Тип молока'), 'Соевое молоко', 80.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Тип молока'), 'Кокосовое молоко', 80.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Тип молока'), 'Миндальное молоко', 80.00),
((SELECT modifier_type_id FROM modifier_types WHERE name = 'Тип молока'), 'Овсяное молоко', 80.00);

-- 5.5. Продукты с начальными остатками
INSERT INTO products (category_id, name, description, stock_quantity) VALUES
-- Кофе
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Эспрессо', 'Крепкий черный кофе', 100),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Капучино', 'Эспрессо с молоком и молочной пенкой', 50),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Латте', 'Эспрессо с большим количеством молока', 50),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Американо', 'Эспрессо с добавлением горячей воды', 80),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Флет-Уайт', 'Кофейный напиток на основе эспрессо и микропены', 30),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Горячий шоколад', 'Напиток на основе какао', 40),
((SELECT category_id FROM categories WHERE name = 'Кофе'), 'Матча-латте', 'Напиток на основе японского чая матча', 40),
-- Чай
((SELECT category_id FROM categories WHERE name = 'Чай'), 'Чай Клюква-можжевельник', 'Травяной чай с ягодами', 30),
((SELECT category_id FROM categories WHERE name = 'Чай'), 'Чай Облепиха-имбирь', 'Тонизирующий чай с имбирем', 30),
((SELECT category_id FROM categories WHERE name = 'Чай'), 'Чай Апельсин-имбирь', 'Фруктовый чай с цитрусами', 30),
-- Раф-кофе
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф классический', 'Кофейный напиток со сливками', 25),
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф кокосовый', 'Кофейный напиток с кокосовыми сливками', 25),
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф халва', 'Кофейный напиток с халвой', 25),
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф арахис', 'Кофейный напиток с арахисовой пастой', 25),
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф цитрус', 'Кофейный напиток с цитрусовыми нотами', 25),
((SELECT category_id FROM categories WHERE name = 'Раф-кофе'), 'Раф масала', 'Кофейный напиток с индийскими специями', 25);

-- 5.6. Варианты продуктов (горячие версии)
INSERT INTO product_variants (product_id, size_id, price, is_available, is_hot_drink)
SELECT 
    p.product_id,
    s.size_id,
    CASE 
        WHEN p.name = 'Эспрессо' AND s.name = '50 мл' THEN 110.00
        WHEN p.name = 'Капучино' AND s.name = 'S' THEN 170.00
        WHEN p.name = 'Капучино' AND s.name = 'M' THEN 200.00
        WHEN p.name = 'Капучино' AND s.name = 'L' THEN 220.00
        WHEN p.name = 'Латте' AND s.name = 'S' THEN 170.00
        WHEN p.name = 'Латте' AND s.name = 'M' THEN 200.00
        WHEN p.name = 'Латте' AND s.name = 'L' THEN 220.00
        WHEN p.name = 'Американо' AND s.name = 'S' THEN 150.00
        WHEN p.name = 'Американо' AND s.name = 'M' THEN 180.00
        WHEN p.name = 'Американо' AND s.name = 'L' THEN 200.00
        WHEN p.name = 'Флет-Уайт' AND s.name = 'S' THEN 200.00
        WHEN p.name = 'Флет-Уайт' AND s.name = 'M' THEN 240.00
        WHEN p.name = 'Горячий шоколад' AND s.name = 'M' THEN 220.00
        WHEN p.name = 'Горячий шоколад' AND s.name = 'L' THEN 240.00
        WHEN p.name = 'Матча-латте' AND s.name = 'M' THEN 220.00
        WHEN p.name = 'Матча-латте' AND s.name = 'L' THEN 240.00
        WHEN p.name LIKE 'Чай%' AND s.name = 'M' THEN 170.00
        WHEN p.name LIKE 'Чай%' AND s.name = 'L' THEN 190.00
        WHEN p.name LIKE 'Раф%' AND s.name = 'M' THEN 240.00
        WHEN p.name LIKE 'Раф%' AND s.name = 'L' THEN 260.00
        ELSE NULL
    END as price,
    CASE 
        WHEN p.name = 'Эспрессо' THEN s.name = '50 мл'
        WHEN p.name = 'Флет-Уайт' AND s.name = 'L' THEN false
        WHEN p.name IN ('Горячий шоколад', 'Матча-латте') AND s.name = 'S' THEN false
        WHEN p.name LIKE 'Чай%' AND s.name = 'S' THEN false
        WHEN p.name LIKE 'Раф%' AND s.name = 'S' THEN false
        WHEN p.name = 'Эспрессо' AND s.name = '50 мл' THEN true
        WHEN p.name LIKE 'Чай%' AND s.name IN ('M', 'L') THEN true
        WHEN p.name LIKE 'Раф%' AND s.name IN ('M', 'L') THEN true
        WHEN p.name NOT LIKE 'Эспрессо%' AND s.name IN ('S', 'M', 'L') THEN true
        ELSE false
    END as is_available,
    true as is_hot_drink
FROM products p
CROSS JOIN sizes s
WHERE NOT (p.name = 'Эспрессо' AND s.name != '50 мл');

-- 5.7. Холодные версии напитков (кроме эспрессо)
INSERT INTO product_variants (product_id, size_id, price, is_available, is_hot_drink)
SELECT 
    pv.product_id,
    pv.size_id,
    CASE 
        -- Для холодных версий можно установить другие цены
        WHEN pv.price IS NOT NULL THEN pv.price + 10.00  -- пример: +10 руб за холодную версию
        ELSE NULL
    END as price,
    pv.is_available,
    false as is_hot_drink
FROM product_variants pv
JOIN products p ON pv.product_id = p.product_id
WHERE p.name != 'Эспрессо' 
  AND pv.price IS NOT NULL
  AND pv.is_available = true;

-- 5.8. Тестовый пользователь
INSERT INTO users (phone_number, full_name) VALUES 
('+79161234567', 'Тестовый Клиент');

-- 5.9. Тестовая корзина
INSERT INTO carts (user_id) VALUES (1);

-- ============================================
-- 6. ЗАВЕРШЕНИЕ ТРАНЗАКЦИИ
-- ============================================
COMMIT;

-- ============================================
-- 7. ПРОВЕРОЧНЫЕ ЗАПРОСЫ (опционально)
-- ============================================

-- 7.1. Проверка структуры базы
SELECT 'users' as table_name, COUNT(*) as row_count FROM users
UNION ALL
SELECT 'products', COUNT(*) FROM products
UNION ALL
SELECT 'product_variants', COUNT(*) FROM product_variants
UNION ALL
SELECT 'modifiers', COUNT(*) FROM modifiers
UNION ALL
SELECT 'categories', COUNT(*) FROM categories
ORDER BY table_name;

-- 7.2. Пример просмотра меню
SELECT 
    c.name as "Категория",
    p.name as "Напиток", 
    s.name as "Размер",
    CASE WHEN pv.is_hot_drink THEN 'Горячий' ELSE 'Холодный' END as "Температура",
    pv.price as "Цена",
    p.stock_quantity as "Остаток"
FROM products p
JOIN categories c ON p.category_id = c.category_id
JOIN product_variants pv ON p.product_id = pv.product_id
JOIN sizes s ON pv.size_id = s.size_id
WHERE pv.is_available = true AND pv.price IS NOT NULL
ORDER BY c.sort_order, p.name, s.display_order, pv.is_hot_drink DESC
LIMIT 10;

-- 7.3. Проверка триггеров (можно выполнить позже)
-- SELECT * FROM carts;
-- SELECT * FROM cart_items;