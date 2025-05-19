#include <drogon/drogon.h>
#include "ContactController.h"

int main() {
    drogon::app().addListener("0.0.0.0", 8080);

    // Подключение к PostgreSQL
    drogon::app().createDbClient("postgresql://user:password@db:5432/contacts_db");

    // Регистрируем роуты
    ContactController::initRoutes();

    drogon::app().run();
}
