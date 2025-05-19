#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ContactController : public drogon::HttpController<ContactController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ContactController::createContact, "/contacts", Post);
        ADD_METHOD_TO(ContactController::getContacts, "/contacts", Get);
    METHOD_LIST_END

    void createContact(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getContacts(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
};
