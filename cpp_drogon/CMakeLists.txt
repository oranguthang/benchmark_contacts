cmake_minimum_required(VERSION 3.14)
project(drogon_contacts_service)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED)
find_package(fmt REQUIRED)

file(GLOB_RECURSE SOURCES "app/*.cpp")

add_executable(server ${SOURCES})

target_link_libraries(server PRIVATE Drogon::Drogon fmt::fmt)
