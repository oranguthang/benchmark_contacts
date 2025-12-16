#include <drogon/drogon.h>
#include "ContactController.h"
#include <cstdlib>
#include <fmt/core.h>

int main() {
    // Determine CPU cores from environment
    int numCPU = 8;
    if (const char* env = std::getenv("CPU_CORES")) {
        try {
            numCPU = std::max(1, std::stoi(env));
        } catch (...) { /* keep default */ }
    }

    // Set Drogon threads to match CPU core count
    drogon::app().setThreadNum(numCPU);

    // Build connection string from environment variables
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

    // Pool size = CPU_CORES * 4
    int poolSize = numCPU * 4;

    // Create connection pool
    auto dbClient = drogon::orm::DbClient::newPgClient(connStr, poolSize, "default");

    // Register controller with this pool
    static ContactController controller(dbClient);
    controller.registerRoutes();

    // Start listener and event loop
    uint16_t port = static_cast<uint16_t>(std::stoi(getEnv("APP_PORT", "8080")));
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().run();

    return 0;
}
