#include <drogon/drogon.h>
#include "ContactController.h"
#include <cstdlib>
#include <fmt/core.h> // Добавляем заголовочный файл fmt

int main() {
    // Установка уровня логирования
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    // Получение параметров из переменных окружения
    auto getEnv = [](const char* name, const char* def) {
        const char* val = std::getenv(name);
        return val ? val : def;
    };

    // Подключение к PostgreSQL
    try {
        // Формируем строку подключения
        std::string connStr = fmt::format(
            "host={} port={} dbname={} user={} password={}",
            getEnv("DB_HOST", "db"),
            getEnv("DB_PORT", "5432"),
            getEnv("DB_NAME", "contacts_db"),
            getEnv("DB_USER", "user"),
            getEnv("DB_PASSWORD", "password")
        );

        // Создаем клиент базы данных
        auto dbClient = drogon::orm::DbClient::newPgClient(
            connStr,
            1,  // Количество соединений
            "default"  // Имя клиента
        );

        // Создаем контроллер
        auto controller = std::make_shared<ContactController>(dbClient);

    } catch (const std::exception &e) {
        LOG_ERROR << "Database connection failed: " << e.what();
        return 1;
    }

    // Запуск сервера
    uint16_t port = static_cast<uint16_t>(
        std::stoi(getEnv("APP_PORT", "8080"))
    );

    LOG_INFO << "Starting server on port " << port;
    drogon::app()
        .addListener("0.0.0.0", port)
        .setThreadNum(4)
        .run();
}
