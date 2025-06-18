cmake_minimum_required(VERSION 3.15)
project(libdb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


message(STATUS "Conan toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Conan generators path: ${CMAKE_PREFIX_PATH}")

find_package(SQLite3 REQUIRED)
find_package(spdlog REQUIRED)

if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 not found! Run 'conan install' first")
endif()
message(STATUS "SQLite3 include dirs: ${SQLite3_INCLUDE_DIRS}")

# Основная библиотека
add_library(libdb STATIC src/db.cpp)
target_include_directories(libdb PUBLIC 
    include
    ${SQLite3_INCLUDE_DIRS}
)

# Явно указываем линковку (важно для Windows)
target_link_libraries(libdb PUBLIC SQLite::SQLite3 spdlog::spdlog)

option(BUILD_TESTING "Build tests" ON)  # Флаг для управления тестами

# Тесты (только если включен BUILD_TESTING)
if(BUILD_TESTING)
    find_package(Catch2 REQUIRED)
    
    # Автоматический поиск всех тестовых файлов
    file(GLOB TEST_SOURCES "test/*.cpp")
    
    add_executable(db_tests ${TEST_SOURCES})
    target_include_directories(db_tests PRIVATE ${Catch2_INCLUDE_DIRS})
    target_link_libraries(db_tests PRIVATE libdb Catch2::Catch2WithMain)

    message(STATUS "Catch2 include dirs: ${Catch2_INCLUDE_DIRS}")
    
    # Подключаем модуль Catch2 для работы с тестами
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
    include(CTest)
    include(Catch)
    catch_discover_tests(db_tests)
endif()
