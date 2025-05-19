#include <drogon/drogon.h>
#include "ContactController.h"

int main() {
    // Установка уровня логирования
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    // Подключение к PostgreSQL
    try {
        // Создаем клиента базы данных с явными параметрами
        auto dbClient = drogon::app().createDbClient(
            "postgresql",  // Тип базы данных
            "db",          // Хост
            5432,          // Порт
            "contacts_db", // Имя базы данных
            "user",        // Имя пользователя
            "password",    // Пароль
            1,            // Количество соединений
            "",           // Файл с сертификатом (не используется)
            "default",     // Имя клиента (для управления несколькими подключениями)
            false,         // auto_auto
            "",            // characterSet (не используется)
            5.0,           // Таймаут соединения
            false          // auto_reconnect
        );

        if (!dbClient) {
            LOG_ERROR << "Failed to create database client";
            return 1;
        }

        // Создаем контроллер и передаем ему клиент базы данных
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
