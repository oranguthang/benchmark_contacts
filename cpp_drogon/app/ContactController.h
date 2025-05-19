#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>

class ContactController : public drogon::HttpController<ContactController> {
public:
    METHOD_LIST_BEGIN
        METHOD_ADD(ContactController::ping, "/ping", drogon::Get);
        METHOD_ADD(ContactController::createContact, "/contacts", drogon::Post);
        METHOD_ADD(ContactController::getContacts, "/contacts", drogon::Get);
    METHOD_LIST_END

    explicit ContactController(const drogon::orm::DbClientPtr &db);

    void ping(const drogon::HttpRequestPtr &req,
              std::function<void (const drogon::HttpResponsePtr &)> &&callback);

    void createContact(const drogon::HttpRequestPtr &req,
                       std::function<void (const drogon::HttpResponsePtr &)> &&callback);

    void getContacts(const drogon::HttpRequestPtr &req,
                     std::function<void (const drogon::HttpResponsePtr &)> &&callback);

private:
    drogon::orm::DbClientPtr dbClient_;
};