# Cafeteria — Matreshka

A cafeteria ordering system with a web interface and console mode. Backend is written in C++20, database - PostgreSQL, frontend - HTML/JS.

---

## Technologies

- C++20
- PostgreSQL + libpqxx
- cpp-httplib - HTTP server
- nlohmann/json - JSON parsing
- Google Test - unit tests
- CMake 3.20+

---

## Build

### Dependencies (macOS)

```bash
brew install libpqxx libpq cpp-httplib nlohmann-json googletest
```

### Build with CMake

```bash
mkdir build && cd build
cmake ..
make
```

---

## Running

### Web mode

Set the database password environment variable before running:

```bash
export DB_PASSWORD=your_password
./Cafeteria
```

The server will start at `http://localhost:8080`

### Console mode

The console application is located in the `feature/variant_with_console_application` branch.

Switch to the branch:

```bash
git checkout feature/variant_with_console_application
```

Run:

```bash
./Cafeteria console
```

---

## Database

Default connection string:

```
host=localhost port=5432 dbname=postgres user=postgres password=admin
```

---

## Project Structure

```
ProjectCafeteria/
├── C++/
│   ├── main.cpp
│   ├── database.cpp
│   ├── database.h
│   ├── server.h
│   ├── coffee_order.h
│   ├── exceptions.h
│   ├── menu_cache.h
│   ├── order_builder.h
│   ├── order_result.h
│   ├── console.h
│   └── tests.cpp
├── PostgreSQL/
│   └── cafeteria_database.sql
├── HTML/
│   └── index.html
└── CMakeLists.txt
```
# Кофейня — Matreshka

Система заказов для кофейни с веб-интерфейсом и консольным режимом. Бэкенд написан на C++20, база данных - PostgreSQL, фронтенд - HTML/JS.

---

## Технологии

- C++20
- PostgreSQL + libpqxx
- cpp-httplib - HTTP сервер
- nlohmann/json - парсинг JSON
- Google Test - unit-тесты
- CMake 3.20+

---

## Сборка

### Зависимости (macOS)

```bash
brew install libpqxx libpq cpp-httplib nlohmann-json googletest
```

### Сборка через CMake

```bash
mkdir build && cd build
cmake ..
make
```

---

## Запуск

### Веб-режим

Перед запуском установить переменную окружения с паролем БД:

```bash
export DB_PASSWORD=ваш_пароль
./Cafeteria
```

Сервер запустится на `http://localhost:8080`

### Консольный режим

Консольное приложение находится в ветке `feature/variant_with_console_application`.

Переключиться на ветку:

```bash
git checkout feature/variant_with_console_application
```

Запуск:

```bash
./Cafeteria console
```

---

## База данных

Строка подключения по умолчанию:

```
host=localhost port=5432 dbname=postgres user=postgres password=admin
```

---

## Структура проекта

```
ProjectCafeteria/
├── C++/
│   ├── main.cpp
│   ├── database.cpp
│   ├── database.h
│   ├── server.h
│   ├── coffee_order.h
│   ├── exceptions.h
│   ├── menu_cache.h
│   ├── order_builder.h
│   ├── order_result.h
│   ├── console.h
│   └── tests.cpp
├── PostgreSQL/
│   └── cafeteria_database.sql
├── HTML/
│   └── index.html
└── CMakeLists.txt
```
