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

void ContactController::getContacts(const HttpRequestPtr &req,
                                    std::function<void (const HttpResponsePtr &)> &&callback) {
    std::string query = "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE 1=1";

    auto external_id = req->getOptionalParameter<int>("external_id");
    auto phone_number = req->getOptionalParameter<std::string>("phone_number");
    auto limit = req->getOptionalParameter<size_t>("limit");
    auto offset = req->getOptionalParameter<size_t>("offset");

    std::vector<const char*> params;
    if (external_id) {
        query += " AND external_id = $1";
        params.push_back(std::to_string(*external_id).c_str());
    }
    if (phone_number) {
        query += " AND phone_number = $2";
        params.push_back(phone_number->c_str());
    }
    if (limit) {
        query += " LIMIT $3";
        params.push_back(std::to_string(*limit).c_str());
    }
    if (offset) {
        query += " OFFSET $4";
        params.push_back(std::to_string(*offset).c_str());
    }

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
        }
    );
}