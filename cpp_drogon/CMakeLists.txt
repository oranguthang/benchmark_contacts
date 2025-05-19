cmake_minimum_required(VERSION 3.16)
project(drogon_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

add_subdirectory(app)
