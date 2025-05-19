cmake_minimum_required(VERSION 3.16)
project(ContactService)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Crow без Boost
add_definitions(-DCROW_MAIN)

# Подключаем внешние зависимости через FetchContent
include(FetchContent)

# Отключаем Boost внутри Crow
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_COMPRESSION OFF CACHE BOOL "" FORCE)
set(CROW_USE_BOOST OFF CACHE BOOL "" FORCE)

# Crow
FetchContent_Declare(
  crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.0+2
)
FetchContent_MakeAvailable(crow)

# libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# nlohmann_json
find_package(nlohmann_json REQUIRED)

# Включаем include директории
include_directories(
    ${PQXX_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Добавляем исполняемый файл
add_executable(server
    main.cpp
    connection_pool.hpp
)

# Линкуем библиотеки
target_link_libraries(server
    ${PQXX_LIBRARIES}
    nlohmann_json::nlohmann_json
    crow
)

# Оптимизация на релиз
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(server PRIVATE -O3 -march=native -flto)
    target_link_options(server PRIVATE -flto)
endif()
