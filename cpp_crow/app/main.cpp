#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <thread>
#include <cstdlib>
#include <iostream>
#include "connection_pool.hpp"

using json = nlohmann::json;

struct Contact {
    std::string id;
    int external_id;
    std::string phone_number;
    std::string date_created;
    std::string date_updated;
};

int main() {
    int numCPU = std::atoi(std::getenv("CPU_CORES") ? std::getenv("CPU_CORES") : "8");
    int poolSize = numCPU * 4;

    std::string dsn = std::getenv("DATABASE_URL") ? std::getenv("DATABASE_URL")
                                                  : "postgresql://user:password@db:5432/contacts_db";

    ConnectionPool pool(dsn, poolSize);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/ping")
    ([] {
        return "pong";
    });

    CROW_ROUTE(app, "/contacts").methods("POST"_method)
    ([&pool](const crow::request& req){
        auto body = json::parse(req.body);
        int external_id = body["external_id"];
        std::string phone_number = body["phone_number"];

        auto conn = pool.acquire();
        pqxx::work txn(*conn);

        txn.exec0("INSERT INTO contacts (external_id, phone_number) VALUES (" +
                   txn.quote(external_id) + ", " + txn.quote(phone_number) + ")");
        pqxx::result res = txn.exec("SELECT id FROM contacts ORDER BY id DESC LIMIT 1");
        std::string id = res[0][0].c_str();
        txn.commit();
        pool.release(conn);

        json result = {
            {"id", id},
            {"external_id", external_id},
            {"phone_number", phone_number},
            {"date_created", crow::utility::date_time::get_iso_utc_now()},
            {"date_updated", crow::utility::date_time::get_iso_utc_now()}
        };

        return crow::response(201, result.dump());
    });

    CROW_ROUTE(app, "/contacts").methods("GET"_method)
    ([&pool](const crow::request& req){
        auto external_id = req.url_params.get("external_id");
        auto phone_number = req.url_params.get("phone_number");
        int limit = req.url_params.get("limit") ? std::atoi(req.url_params.get("limit")) : 10000;
        int offset = req.url_params.get("offset") ? std::atoi(req.url_params.get("offset")) : 0;

        if (limit > 10000) limit = 10000;

        std::string query = "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE TRUE";
        if (external_id)
            query += " AND external_id = " + std::string(external_id);
        if (phone_number)
            query += " AND phone_number = '" + std::string(phone_number) + "'";
        query += " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto conn = pool.acquire();
        pqxx::work txn(*conn);
        pqxx::result res = txn.exec(query);
        txn.commit();
        pool.release(conn);

        json result = json::array();
        for (const auto& row : res) {
            result.push_back({
                {"id", row["id"].c_str()},
                {"external_id", row["external_id"].as<int>()},
                {"phone_number", row["phone_number"].c_str()},
                {"date_created", row["date_created"].c_str()},
                {"date_updated", row["date_updated"].c_str()}
            });
        }

        return crow::response(200, result.dump());
    });

    std::cout << "Server is running on :8080 with " << numCPU << " CPU cores and pool size " << poolSize << std::endl;
    app.port(8080).concurrency(numCPU).run();
}
