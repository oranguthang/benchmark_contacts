#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>

class ContactController : public drogon::HttpController<ContactController> {
public:
    explicit ContactController(const drogon::orm::DbClientPtr &db);

    void ping(const drogon::HttpRequestPtr &req,
              std::function<void (const drogon::HttpResponsePtr &)> &&callback);

    void createContact(const drogon::HttpRequestPtr &req,
                       std::function<void (const drogon::HttpResponsePtr &)> &&callback);

    void getContacts(const drogon::HttpRequestPtr &req,
                     std::function<void (const drogon::HttpResponsePtr &)> &&callback);

    static void initRoutes();
private:
    drogon::orm::DbClientPtr dbClient_;
};
