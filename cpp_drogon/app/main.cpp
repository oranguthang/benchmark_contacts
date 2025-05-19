#include <drogon/drogon.h>
#include "ContactController.h"
#include <cstdlib> // для getenv

int main() {
    // Установка уровня логирования
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    // Получаем параметры БД из переменных окружения или используем значения по умолчанию
    const char* dbHost = std::getenv("DB_HOST");
    const char* dbPort = std::getenv("DB_PORT");
    const char* dbName = std::getenv("DB_NAME");
    const char* dbUser = std::getenv("DB_USER");
    const char* dbPass = std::getenv("DB_PASSWORD");

    // Подключение к PostgreSQL
    try {
        // Создаем конфигурацию базы данных
        drogon::orm::DbConfig dbConfig;
        dbConfig.host = dbHost ? dbHost : "db";
        dbConfig.port = dbPort ? std::stoi(dbPort) : 5432;
        dbConfig.dbname = dbName ? dbName : "contacts_db";
        dbConfig.user = dbUser ? dbUser : "user";
        dbConfig.password = dbPass ? dbPass : "password";
        dbConfig.connectionNumber = 1;
        dbConfig.clientName = "default";

        // Добавляем клиент базы данных
        drogon::app().addDbClient(dbConfig);

        // Получаем клиент базы данных
        auto dbClient = drogon::app().getDbClient("default");
        if (!dbClient) {
            LOG_ERROR << "Failed to get database client";
            return 1;
        }

        // Создаем контроллер
        auto controller = std::make_shared<ContactController>(dbClient);

    } catch (const std::exception &e) {
        LOG_ERROR << "Database connection failed: " << e.what();
        return 1;
    }

    // Получаем порт из переменных окружения
    const char* portStr = std::getenv("APP_PORT");
    uint16_t port = portStr ? std::stoi(portStr) : 8080;

    // Добавляем слушатель
    drogon::app()
        .addListener("0.0.0.0", port)
        .setThreadNum(4)
        .run();
}
