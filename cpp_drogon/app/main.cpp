#include <drogon/drogon.h>
#include "ContactController.h"
#include <cstdlib>
#include <fmt/core.h>

int main() {
    drogon::app().setLogLevel(trantor::Logger::kDebug);

    auto getEnv = [](const char* name, const char* def) {
        const char* val = std::getenv(name);
        return val ? val : def;
    };

    std::string connStr = fmt::format(
        "host={} port={} dbname={} user={} password={}",
        getEnv("DB_HOST", "db"),
        getEnv("DB_PORT", "5432"),
        getEnv("DB_NAME", "contacts_db"),
        getEnv("DB_USER", "user"),
        getEnv("DB_PASSWORD", "password")
    );

    auto dbClient = drogon::orm::DbClient::newPgClient(connStr, 1, "default");

    static ContactController controller(dbClient);
    controller.registerRoutes();

    uint16_t port = static_cast<uint16_t>(std::stoi(getEnv("APP_PORT", "8080")));

    LOG_INFO << "Starting server on port " << port;
    drogon::app()
        .addListener("0.0.0.0", port)
        .setThreadNum(4)
        .run();
}
