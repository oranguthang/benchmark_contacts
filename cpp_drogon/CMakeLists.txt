cmake_minimum_required(VERSION 3.14)
project(drogon_contacts_service)

set(CMAKE_CXX_STANDARD 17)

# Ищем Drogon (он сам найдет Jsoncpp)
find_package(Drogon REQUIRED COMPONENTS jsoncpp)

add_executable(server
    app/main.cpp
    app/ContactController.cpp
)

target_include_directories(server PRIVATE app)

# Линкуем только Drogon, он сам подключит Jsoncpp
target_link_libraries(server PRIVATE Drogon::Drogon)