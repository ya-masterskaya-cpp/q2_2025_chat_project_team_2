# common/chat_server_lib/CMakeLists.txt

project(ChatServerLib)

add_library(ChatServerLib STATIC
    src/Server.cpp
)

target_include_directories(ChatServerLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Перечисляем зависимости библиотеки
target_link_libraries(ChatServerLib PUBLIC
    libdb # Зависит от библиотеки БД
    Boost::boost
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    add_struct
)