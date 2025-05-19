cmake_minimum_required(VERSION 3.14)
project(drogon_contacts_service)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED COMPONENTS jsoncpp orm_postgresql)

add_executable(server
    app/main.cpp
    app/ContactController.cpp
)

target_include_directories(server PRIVATE app)
target_link_libraries(server PRIVATE Drogon::Drogon)
