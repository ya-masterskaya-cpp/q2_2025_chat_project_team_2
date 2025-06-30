# common/libdb/CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(libdb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(spdlog REQUIRED)

# Основная библиотека
add_library(libdb STATIC src/db.cpp)

target_include_directories(libdb PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # Для правильной установки пакета в будущем
)

target_link_libraries(libdb PUBLIC SQLite::SQLite3 spdlog::spdlog)

option(BUILD_TESTING "Build tests" ON)  # Флаг для управления тестами

# Тесты (только если включен BUILD_TESTING)
if(BUILD_TESTING)
    find_package(Catch2 REQUIRED)
    
    add_executable(db_and_server_tests  
        test/test.cpp 
	# test/server_tests.cpp
        # Добавляйте новые файлы тестов сюда
    )
    
    target_link_libraries(db_and_server_tests  PRIVATE
	libdb
	ChatServerLib 
	Catch2::Catch2WithMain
    )

    # Подключаем модуль Catch2 для работы с тестами
    include(CTest)
    include(Catch)
    catch_discover_tests(db_and_server_tests)
endif()
