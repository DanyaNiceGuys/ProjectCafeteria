#include "server.h"
#include "database.h"
#include "menu_cache.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

int main(int argc, char* argv[]) {

    const char* dbPassword = std::getenv("DB_PASSWORD");
    if (!dbPassword) {
        dbPassword = "admin";
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

    const std::string htmlPath = "/Users/Chlenoedov/Desktop/proj/HTML/index.html"; // set ur path

    CafeteriaServer server(db, cache, htmlPath);
    server.run("localhost", 8080);

    return 0;
}
