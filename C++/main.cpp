#include "server.h"
#include "database.h"
#include "menu_cache.h"
#include "console.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

//   1. Читаем пароль БД из переменной окружения DB_PASSWORD
//   2. Подключаемся к PostgreSQL (RAII - соединение закроется само при выходе)
//   3. Создаём MenuCache - кэш меню в памяти
//   4. Находим index.html относительно исполняемого файла (работает на любой машине)
//   5. Запускаем HTTP сервер на localhost:8080
//   6. Режим ./Cafeteria console - запуск терминального интерфейса (ConsoleUI)


int main(int argc, char* argv[]) {

    // Пароль читаем из переменной окружения - не храним его в коде.
    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        dbPassword = "admin"; // значение по умолчанию для локальной разработки
    }

    std::string conninfo =
            "host=localhost port=5432 dbname=postgres user=postgres password=" +
            std::string(dbPassword);

    // Database владеет соединением через unique_ptr (RAII).
    // При выходе из main или при исключении соединение закроется автоматически.
    Database db(conninfo);
    try {
        db.connect();
    } catch (const ConnectionException& e) {
        std::cerr << "Не удалось подключиться к БД: " << e.what() << "\n";
        return 1;
    }

    // Кэш меню — данные загружаются из БД один раз, дальше отдаются из памяти
    MenuCache cache;

    // Режим консольного интерфейса: ./Cafeteria console
    if (argc > 1 && std::string(argv[1]) == "console") {
        ConsoleUI ui(db, cache);
        ui.run();
        return 0;
    }

    // Путь к index.html определяется относительно исполняемого файла.
    // Работает на любой машине независимо от того где лежит проект.

    const std::string htmlPath = "/Users//Desktop/proj/HTML/index.html"; // set ur path

    CafeteriaServer server(db, cache, htmlPath);
    server.run("localhost", 8080);

    return 0;
}