#include "ContactController.h"
#include <json/json.h>

void ContactController::createContact(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpJsonResponse(Json::Value{ {"error", "Invalid JSON"} });
        resp->setStatusCode(k400BadRequest);
        return callback(resp);
    }

    auto name = (*json)["name"].asString();
    auto phone = (*json)["phone"].asString();

    Json::Value result;
    result["status"] = "created";
    result["name"] = name;
    result["phone"] = phone;

    auto resp = HttpResponse::newHttpJsonResponse(result);
    callback(resp);
}

void ContactController::getContacts(const HttpRequestPtr &, std::function<void (const HttpResponsePtr &)> &&callback) {
    Json::Value result;
    Json::Value contact;
    contact["name"] = "Test User";
    contact["phone"] = "+1234567890";
    result.append(contact);

    auto resp = HttpResponse::newHttpJsonResponse(result);
    callback(resp);
}
