cmake_minimum_required(VERSION 3.14)
project(drogon_contacts_service)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED)

add_executable(server main.cpp)
target_link_libraries(server PRIVATE Drogon::Drogon)
