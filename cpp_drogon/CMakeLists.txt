cmake_minimum_required(VERSION 3.14)
project(drogon_contacts_service)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED)

# Собираем исполняемый файл из исходников в папке app
add_executable(server
    app/main.cpp
    app/ContactController.cpp
)

# Подключаем заголовочные файлы из папки app
target_include_directories(server PRIVATE app)

target_link_libraries(server PRIVATE Drogon::Drogon)
