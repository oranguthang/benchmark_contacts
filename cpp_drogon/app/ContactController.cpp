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
                                    std::function<void(const std::shared_ptr<HttpResponse>&)>&& callback)
{
    auto client = drogon::app().getDbClient();

    std::string sql = "SELECT id, external_id, name, phone FROM contacts WHERE 1=1";

    // Динамическая сборка фильтров
    if (req->getParameter("external_id") != "") {
        sql += " AND external_id = '" + client->sqlEscape(req->getParameter("external_id")) + "'";
    }

    if (req->getParameter("phone") != "") {
        sql += " AND phone = '" + client->sqlEscape(req->getParameter("phone")) + "'";
    }

    // Лимит и оффсет по умолчанию
    int limit = 100;
    int offset = 0;

    if (req->getParameter("limit") != "") {
        limit = std::stoi(req->getParameter("limit"));
    }

    if (req->getParameter("offset") != "") {
        offset = std::stoi(req->getParameter("offset"));
    }

    sql += " ORDER BY id";
    sql += " LIMIT " + std::to_string(limit);
    sql += " OFFSET " + std::to_string(offset);

    // Выполнение запроса
    client->execSqlAsync(
        sql,
        [callback](const drogon::orm::Result &result) {
            // Формируем JSON-ответ
            Json::Value response(Json::arrayValue);
            for (const auto &row : result) {
                Json::Value contact;
                contact["id"] = row["id"].as<int>();
                contact["external_id"] = row["external_id"].as<std::string>();
                contact["name"] = row["name"].as<std::string>();
                contact["phone"] = row["phone"].as<std::string>();
                response.append(contact);
            }

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        },
        [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "Database error: " << e.base().what();
        }
    );
}
