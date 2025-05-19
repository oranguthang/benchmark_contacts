cmake_minimum_required(VERSION 3.16)
project(ContactService)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DCROW_MAIN)

include(FetchContent)

# Crow с master ветки (где нормально отключается Boost)
FetchContent_Declare(
  crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG master
)
# Отключаем всё лишнее перед MakeAvailable
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_COMPRESSION OFF CACHE BOOL "" FORCE)
set(CROW_USE_BOOST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(crow)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
find_package(nlohmann_json REQUIRED)

include_directories(
    ${PQXX_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(server
    main.cpp
    connection_pool.hpp
)

target_link_libraries(server
    ${PQXX_LIBRARIES}
    nlohmann_json::nlohmann_json
    crow
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(server PRIVATE -O3 -march=native -flto)
    target_link_options(server PRIVATE -flto)
endif()
