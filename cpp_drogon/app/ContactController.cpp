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
    auto client = drogon::app().getDbClient();
    if (!client) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database connection failed");
        callback(resp);
        return;
    }

    // Базовый запрос
    std::string sql = "SELECT id, external_id, name, phone FROM contacts WHERE 1=1";

    // Параметры для фильтрации
    std::string external_id_filter;
    std::string phone_filter;
    std::string external_id;
    std::string phone;

    // Добавляем фильтры
    if (!(external_id = req->getParameter("external_id")).empty()) {
        external_id_filter = " AND external_id = $1";
        sql += external_id_filter;
    }

    if (!(phone = req->getParameter("phone")).empty()) {
        phone_filter = external_id_filter.empty() ? " AND phone = $1" : " AND phone = $2";
        sql += phone_filter;
    }

    // Лимит и оффсет
    int limit = 100;
    int offset = 0;

    try {
        if (auto limit_param = req->getParameter("limit"); !limit_param.empty()) {
            limit = std::stoi(limit_param);
        }
        if (auto offset_param = req->getParameter("offset"); !offset_param.empty()) {
            offset = std::stoi(offset_param);
        }
    } catch (const std::exception& e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid limit or offset value");
        callback(resp);
        return;
    }

    sql += " ORDER BY id LIMIT " + std::to_string(limit) +
           " OFFSET " + std::to_string(offset);

    // Выполняем запрос с правильной передачей параметров
    if (!external_id.empty() && !phone.empty()) {
        client->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result &result) {
                Json::Value response(Json::arrayValue);
                for (const auto &row : result) {
                    Json::Value contact;
                    contact["id"] = row["id"].as<int>();
                    contact["external_id"] = row["external_id"].as<std::string>();
                    contact["name"] = row["name"].as<std::string>();
                    contact["phone"] = row["phone"].as<std::string>();
                    response.append(contact);
                }
                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            },
            [callback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Database error: " << e.base().what();
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Database error occurred");
                callback(resp);
            },
            external_id,
            phone
        );
    }
    else if (!external_id.empty()) {
        client->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result &result) { /* same as above */ },
            [callback](const drogon::orm::DrogonDbException &e) { /* same as above */ },
            external_id
        );
    }
    else if (!phone.empty()) {
        client->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result &result) { /* same as above */ },
            [callback](const drogon::orm::DrogonDbException &e) { /* same as above */ },
            phone
        );
    }
    else {
        client->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result &result) { /* same as above */ },
            [callback](const drogon::orm::DrogonDbException &e) { /* same as above */ }
        );
    }
}
