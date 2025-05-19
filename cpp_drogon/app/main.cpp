#include <drogon/drogon.h>
#include "ContactController.h"
#include <cstdlib>
#include <fmt/core.h>

int main() {
    // Определяем число ядер из окружения
    int numCPU = 8;
    if (const char* env = std::getenv("CPU_CORES")) {
        try {
            numCPU = std::max(1, std::stoi(env));
        } catch (...) { /* оставляем дефолт */ }
    }

    // Устанавливаем Drogon-потоки под количество ядер
    drogon::app().setThreadNum(numCPU);

    // Формируем строку подключения из переменных окружения
    auto getEnv = [](const char* name, const char* def) {
        const char* v = std::getenv(name);
        return v ? v : def;
    };
    std::string connStr = fmt::format(
        "host={} port={} dbname={} user={} password={}",
        getEnv("DB_HOST", "db"),
        getEnv("DB_PORT", "5432"),
        getEnv("DB_NAME", "contacts_db"),
        getEnv("DB_USER", "user"),
        getEnv("DB_PASSWORD", "password")
    );

    // Размер пула = CPU_CORES * 4
    int poolSize = numCPU * 4;

    // Создаем пул коннектов
    auto dbClient = drogon::orm::DbClient::newPgClient(connStr, poolSize, "default");

    // Регистрируем контроллер с этим пулом
    static ContactController controller(dbClient);
    controller.registerRoutes();

    // Запускаем слушатель и цикл обработки
    uint16_t port = static_cast<uint16_t>(std::stoi(getEnv("APP_PORT", "8080")));
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().run();

    return 0;
}
