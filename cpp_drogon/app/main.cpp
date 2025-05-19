#include <drogon/drogon.h>
#include "ContactController.h"

int main() {
    // Установка уровня логирования
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    // Подключение к PostgreSQL
    try {
        // Создаем конфигурацию базы данных
        drogon::orm::DbConfig dbConfig;
        dbConfig.host = "db";
        dbConfig.port = 5432;
        dbConfig.dbname = "contacts_db";
        dbConfig.user = "user";
        dbConfig.password = "password";
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

    // Добавляем слушатель
    drogon::app()
        .addListener("0.0.0.0", 8080)
        .setThreadNum(4)
        .run();
}
