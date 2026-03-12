#pragma once
#include <stdexcept>
#include <string>

// Базовый класс - все исключения проекта наследуют от него
class CafeteriaException : public std::runtime_error {
public:
    explicit CafeteriaException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// Ошибки при работе с базой данных
class DatabaseException : public CafeteriaException {
public:
    explicit DatabaseException(const std::string& msg)
        : CafeteriaException("DB Error: " + msg) {}
};

// Не удалось открыть соединение с PostgreSQL
class ConnectionException : public DatabaseException {
public:
    explicit ConnectionException(const std::string& conninfo)
        : DatabaseException("Cannot connect: " + conninfo) {}
};

// Некорректные данные заказа (пустое имя, нет позиций и т.д.)
class OrderValidationException : public CafeteriaException {
public:
    explicit OrderValidationException(const std::string& msg)
        : CafeteriaException("Validation: " + msg) {}
};
