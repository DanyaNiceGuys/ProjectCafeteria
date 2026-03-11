#include "server.h"
#include "database.h"
#include "menu_cache.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

int main(int argc, char* argv[]) {

    // Пароль читаем из переменной окружения — не храним его в коде.
    // Перед запуском выполнить: export DB_PASSWORD=твой_пароль
    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        dbPassword = "admin"; // значение по умолчанию для локальной разработки
        std::cerr << "[WARN] DB_PASSWORD не задан, используется значение по умолчанию\n";
    }

    std::string conninfo =
        "host=localhost port=5432 dbname=postgres user=postgres password=" +
        std::string(dbPassword);

    Database db(conninfo);
    try {
        db.connect();
    } catch (const ConnectionException& e) {
        std::cerr << "Не удалось подключиться к БД: " << e.what() << "\n";
        return 1;
    }

    MenuCache cache;

    namespace fs = std::filesystem;
    fs::path    exeDir   = fs::canonical(argv[0]).parent_path();
    std::string htmlPath = (exeDir.parent_path() / "HTML" / "index.html").string();
    std::cout << "[INFO] HTML: " << htmlPath << "\n";

    CafeteriaServer server(db, cache, htmlPath);
    server.run("localhost", 8080);

    return 0;
}
