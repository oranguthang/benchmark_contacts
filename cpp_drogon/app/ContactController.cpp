#include "ContactController.h"
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>
#include <json/json.h>

using namespace drogon;
using namespace drogon::orm;

ContactController::ContactController(const DbClientPtr &db)
    : dbClient_(db)
{}

void ContactController::registerRoutes() {
    app().registerHandler("/ping",
        [this](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            this->ping(req, std::move(callback));
        },
        {Get});

    app().registerHandler("/contacts",
        [this](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            if (req->getMethod() == Post)
                this->createContact(req, std::move(callback));
            else
                this->getContacts(req, std::move(callback));
        },
        {Get, Post});
}

void ContactController::ping(const HttpRequestPtr &req,
                             std::function<void (const HttpResponsePtr &)> &&callback) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setContentTypeCode(CT_TEXT_PLAIN);
    resp->setBody("pong");
    callback(resp);
}

void ContactController::createContact(const HttpRequestPtr &req,
                                      std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("external_id") || !json->isMember("phone_number") ||
        !(*json)["external_id"].isInt() || !(*json)["phone_number"].isString()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        resp->setBody("Missing/invalid fields");
        callback(resp);
        return;
    }

    int external_id = (*json)["external_id"].asInt();
    std::string phone_number = (*json)["phone_number"].asString();

    dbClient_->execSqlAsync(
        "INSERT INTO contacts (external_id, phone_number) VALUES ($1, $2) ON CONFLICT DO NOTHING RETURNING id, external_id, phone_number, date_created, date_updated",
        [callback](const Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k409Conflict);
                resp->setContentTypeCode(CT_TEXT_PLAIN);
                resp->setBody("Contact already exists");
                callback(resp);
                return;
            }

            auto row = r[0];
            Json::Value result;
            result["id"] = row["id"].as<std::string>();
            result["external_id"] = row["external_id"].as<int>();
            result["phone_number"] = row["phone_number"].as<std::string>();
            result["date_created"] = row["date_created"].as<std::string>();
            result["date_updated"] = row["date_updated"].as<std::string>();

            auto resp = HttpResponse::newHttpJsonResponse(result);
            resp->setStatusCode(k201Created);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setContentTypeCode(CT_TEXT_PLAIN);
            resp->setBody(std::string("DB error: ") + e.base().what());
            callback(resp);
        },
        external_id, phone_number
    );
}

void ContactController::getContacts(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& callback)
{
    // Simple "SQL-injection" protection for strings
    auto escape = [](const std::string &s) {
        std::string r;
        r.reserve(s.size());
        for (char c : s) {
            if (c == '\'') r += "''";
            else r += c;
        }
        return r;
    };

    // Get client from controller field, not from app()
    auto client = dbClient_;
    if (!client) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database connection failed");
        callback(resp);
        return;
    }

    // Build the SQL query
    std::string sql =
        "SELECT id, external_id, phone_number, date_created, date_updated "
        "FROM contacts WHERE 1=1";

    if (auto ext = req->getParameter("external_id"); !ext.empty()) {
        sql += " AND external_id = '" + escape(ext) + "'";
    }
    if (auto ph = req->getParameter("phone_number"); !ph.empty()) {
        sql += " AND phone_number = '" + escape(ph) + "'";
    }

    int limit = 10000, offset = 0;
    try {
        if (auto l = req->getParameter("limit"); !l.empty())
            limit = std::stoi(l);
        if (auto o = req->getParameter("offset"); !o.empty())
            offset = std::stoi(o);
    } catch (...) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid limit or offset");
        callback(resp);
        return;
    }

    sql += " ORDER BY id"
           " LIMIT " + std::to_string(limit) +
           " OFFSET " + std::to_string(offset);

    // Callbacks
    auto onSuccess = [callback](const drogon::orm::Result &rows) {
        Json::Value arr(Json::arrayValue);
        for (auto &r : rows) {
            Json::Value c;
            c["id"]            = r["id"].as<std::string>();
            c["external_id"]   = r["external_id"].as<int>();
            c["phone_number"]  = r["phone_number"].as<std::string>();
            c["date_created"]  = r["date_created"].as<std::string>();
            c["date_updated"]  = r["date_updated"].as<std::string>();
            arr.append(c);
        }
        callback(HttpResponse::newHttpJsonResponse(arr));
    };
    auto onError = [callback](const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "DB error: " << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error occurred");
        callback(resp);
    };

    // Execute the query
    client->execSqlAsync(sql, onSuccess, onError);
}
