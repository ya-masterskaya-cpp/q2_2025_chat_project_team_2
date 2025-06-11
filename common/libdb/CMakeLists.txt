cmake_minimum_required(VERSION 3.15)
project(libdb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SQLite3
add_library(sqlite3 STATIC sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3)

# libdb (ваша обёртка)
add_library(libdb STATIC src/db.cpp)
# add_library(libdb STATIC src/db.cpp src/stmt.cpp)
target_include_directories(libdb PUBLIC include)
target_link_libraries(libdb PUBLIC sqlite3)

# Тестовое приложение
add_executable(test_app test/main.cpp)
target_link_libraries(test_app PRIVATE libdb)

