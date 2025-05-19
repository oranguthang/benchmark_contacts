#include <drogon/drogon.h>
#include "ContactController.h"

int main() {
    // Установка уровня логирования
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    // Подключение к PostgreSQL
    try {
        auto dbClient = drogon::app().createDbClient("postgresql://user:password@db:5432/contacts_db");
        if (!dbClient) {
            LOG_ERROR << "Failed to create database client";
            return 1;
        }
    } catch (const std::exception &e) {
        LOG_ERROR << "Database connection failed: " << e.what();
        return 1;
    }

    // Регистрируем роуты
    ContactController::initRoutes();

    // Добавляем слушатель
    drogon::app()
        .addListener("0.0.0.0", 8080)
        .setThreadNum(4)
        .run();
}
