#include "ContactController.h"
#include <drogon/utils/Utilities.h>
#include <json/json.h>

using namespace drogon;
using namespace drogon::orm;

ContactController::ContactController(const DbClientPtr &db)
    : dbClient_(db)
{}

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
        "INSERT INTO contacts (external_id, phone_number) VALUES ($1, $2) ON CONFLICT DO NOTHING RETURNING id, date_created, date_updated",
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

void ContactController::getContacts(const HttpRequestPtr &req,
                                    std::function<void (const HttpResponsePtr &)> &&callback) {
    std::string query = "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE 1=1";
    std::vector<std::string> whereParams;
    std::vector<std::string> limitOffsetParams;

    if (auto external_id = req->getParameter("external_id"); !external_id.empty()) {
        query += " AND external_id = $" + std::to_string(whereParams.size() + 1);
        whereParams.push_back(external_id);
    }

    if (auto phone = req->getParameter("phone_number"); !phone.empty()) {
        query += " AND phone_number = $" + std::to_string(whereParams.size() + 1);
        whereParams.push_back(phone);
    }

    if (auto limit = req->getParameter("limit"); !limit.empty()) {
        query += " LIMIT $" + std::to_string(whereParams.size() + 1);
        limitOffsetParams.push_back(limit);
    }

    if (auto offset = req->getParameter("offset"); !offset.empty()) {
        query += " OFFSET $" + std::to_string(whereParams.size() + limitOffsetParams.size() + 1);
        limitOffsetParams.push_back(offset);
    }

    std::vector<std::string> allParams;
    allParams.insert(allParams.end(), whereParams.begin(), whereParams.end());
    allParams.insert(allParams.end(), limitOffsetParams.begin(), limitOffsetParams.end());

    dbClient_->execSqlAsync(
        query,
        [callback](const Result &r) {
            Json::Value result = Json::arrayValue;
            for (auto row : r) {
                Json::Value contact;
                contact["id"] = row["id"].as<std::string>();
                contact["external_id"] = row["external_id"].as<int>();
                contact["phone_number"] = row["phone_number"].as<std::string>();
                contact["date_created"] = row["date_created"].as<std::string>();
                contact["date_updated"] = row["date_updated"].as<std::string>();
                result.append(contact);
            }

            auto resp = HttpResponse::newHttpJsonResponse(result);
            callback(resp);
        },
        [callback](const DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setContentTypeCode(CT_TEXT_PLAIN);
            resp->setBody(std::string("DB error: ") + e.base().what());
            callback(resp);
        },
        allParams
    );
}

void ContactController::initRoutes() {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        LOG_ERROR << "Database client is not initialized!";
        return;
    }

    auto controller = std::make_shared<ContactController>(dbClient);

    drogon::app().registerHandler("/ping", [controller](const HttpRequestPtr &req,
                                                        std::function<void (const HttpResponsePtr &)> callback) {
        controller->ping(req, std::move(callback));
    },
    {Get});

    drogon::app().registerHandler("/contacts", [controller](const HttpRequestPtr &req,
                                                            std::function<void (const HttpResponsePtr &)> callback) {
        if (req->method() == Post) {
            controller->createContact(req, std::move(callback));
        } else if (req->method() == Get) {
            controller->getContacts(req, std::move(callback));
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k405MethodNotAllowed);
            callback(resp);
        }
    },
    {Get, Post});
}