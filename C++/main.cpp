#include "server.h"
#include "database.h"
#include "menu_cache.h"
#include "console_ui.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>
int main(int argc, char* argv[]) {

    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        dbPassword = "admin";
        std::cerr << "[WARN] DB_PASSWORD не задан\n";
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

    // Консольный режим
    if (argc > 1 && std::string(argv[1]) == "console") {
        std::cout << "Запуск в консольном режиме...\n";
        ConsoleUI ui(db, cache);
        ui.run();
        return 0;
    }

    // Веб-сервер
    namespace fs = std::filesystem;
    fs::path    exeDir   = fs::canonical(argv[0]).parent_path();
    std::string htmlPath = (exeDir.parent_path() / "HTML" / "index.html").string();
    std::cout << "[INFO] HTML: " << htmlPath << "\n";
    std::cout << "Для консольного режима: ./Cafeteria console\n";

    CafeteriaServer server(db, cache, htmlPath);
    server.run("localhost", 8080);

    return 0;
}
