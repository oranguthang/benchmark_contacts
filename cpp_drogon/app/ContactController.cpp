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
    using namespace drogon;
    using namespace drogon::orm;

    std::string query = "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE 1=1";
    std::vector<std::any> params;
    int paramIndex = 1;

    if (auto externalId = req->getOptionalParameter<int>("external_id")) {
        query += " AND external_id = $" + std::to_string(paramIndex++);
        params.emplace_back(*externalId);
    }
    if (auto phoneNumber = req->getOptionalParameter<std::string>("phone_number")) {
        query += " AND phone_number = $" + std::to_string(paramIndex++);
        params.emplace_back(*phoneNumber);
    }
    if (auto limit = req->getOptionalParameter<size_t>("limit")) {
        query += " LIMIT $" + std::to_string(paramIndex++);
        params.emplace_back(static_cast<int>(*limit));
    }
    if (auto offset = req->getOptionalParameter<size_t>("offset")) {
        query += " OFFSET $" + std::to_string(paramIndex++);
        params.emplace_back(static_cast<int>(*offset));
    }

    auto onSuccess = [callback](const Result& r) {
        Json::Value result = Json::arrayValue;
        for (const auto& row : r) {
            Json::Value contact;
            contact["id"] = row["id"].as<std::string>();
            contact["external_id"] = row["external_id"].as<int>();
            contact["phone_number"] = row["phone_number"].as<std::string>();
            contact["date_created"] = row["date_created"].as<std::string>();
            contact["date_updated"] = row["date_updated"].as<std::string>();
            result.append(contact);
        }
        callback(HttpResponse::newHttpJsonResponse(result));
    };

    auto onError = [callback](const DrogonDbException& e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        resp->setBody(std::string("DB error: ") + e.base().what());
        callback(resp);
    };

    // Вызываем execSqlAsync с параметрами по количеству
    if (params.empty()) {
        dbClient_->execSqlAsync(query, onSuccess, onError);
    } else if (params.size() == 1) {
        dbClient_->execSqlAsync(query, onSuccess, onError, params[0]);
    } else if (params.size() == 2) {
        dbClient_->execSqlAsync(query, onSuccess, onError, params[0], params[1]);
    } else if (params.size() == 3) {
        dbClient_->execSqlAsync(query, onSuccess, onError, params[0], params[1], params[2]);
    } else if (params.size() == 4) {
        dbClient_->execSqlAsync(query, onSuccess, onError, params[0], params[1], params[2], params[3]);
    } else {
        // максимум 4 параметра по нашей логике, иначе ошибка
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Too many parameters");
        callback(resp);
    }
}
