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
        // Создаем конфигурацию базы данных для PostgreSQL
        drogon::orm::PostgresConfig pgConfig;
        pgConfig.host = dbHost ? dbHost : "db";
        pgConfig.port = dbPort ? std::stoi(dbPort) : 5432;
        pgConfig.dbname = dbName ? dbName : "contacts_db";
        pgConfig.user = dbUser ? dbUser : "user";
        pgConfig.password = dbPass ? dbPass : "password";
        pgConfig.connectionNumber = 1;
        pgConfig.clientName = "default";

        // Добавляем клиент базы данных
        drogon::app().addDbClient(pgConfig);

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