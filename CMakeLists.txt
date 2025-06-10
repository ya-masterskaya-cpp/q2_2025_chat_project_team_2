cmake_minimum_required(VERSION 3.15)

project(irc_chat CXX)
set(CMAKE_CXX_STANDARD 17)

# Подключение Conan
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake) # Генерируется CMakeToolchain

# Поиск зависимостей
find_package(Boost REQUIRED COMPONENTS system CONFIG)
find_package(wxWidgets REQUIRED COMPONENTS core base adv CONFIG)
find_package(SQLite3 REQUIRED CONFIG)
find_package(OpenSSL REQUIRED CONFIG) 

# дополнительные структуры
add_library(add_struct INTERFACE 
	common/add_struct/add_struct.h
	common/config/config.h
)
target_include_directories(add_struct INTERFACE
    ${PROJECT_SOURCE_DIR}/common/add_struct
    ${PROJECT_SOURCE_DIR}/common/config
)
target_link_libraries(add_struct INTERFACE 
	wxWidgets::wxWidgets 
)

# для работы с SSL
add_library(SSL_Lib STATIC 
	common/ssl/ssl.h
	common/ssl/ssl.cpp
)
target_include_directories(SSL_Lib PUBLIC 
    ${PROJECT_SOURCE_DIR}/common/ssl
)
target_link_libraries(SSL_Lib PUBLIC openssl::openssl)

# для работы с базой данных
add_library(DatabaseLib STATIC 
	common/database/database.h
	common/database/database.cpp
)
target_include_directories(DatabaseLib PUBLIC 
    ${PROJECT_SOURCE_DIR}/common/database
)
target_link_libraries(DatabaseLib PUBLIC SQLite::SQLite3)

# для работы с client_logic
add_library(ClientLogicLib STATIC 
	src/client/ChatClient.h
	src/client/ChatClient.cpp
)
target_include_directories(ClientLogicLib PUBLIC 
    ${PROJECT_SOURCE_DIR}/src/client
)
target_link_libraries(ClientLogicLib PUBLIC 
	SSL_Lib
	add_struct
)

# для работы с gui
add_library(GUILib STATIC 
	common/gui/MainWindow.h
	common/gui/MainWindow.cpp
	common/gui/LoginDialog.h
	common/gui/LoginDialog.cpp
	common/gui/BBCodeUtils.h
	common/gui/BBCodeUtils.cpp
	common/gui/RegisterDialog.h
	common/gui/RegisterDialog.cpp
	common/gui/ListSelectionDialog.h
	common/gui/ListSelectionDialog.cpp
)
target_include_directories(GUILib PUBLIC 
    ${PROJECT_SOURCE_DIR}/common/gui
)
target_link_libraries(GUILib PUBLIC 
	DatabaseLib 
	wxWidgets::wxWidgets 
	ClientLogicLib
)

# Создание цели
add_executable(client 
	src/client/main.cpp
	src/client/ClientApp.h
	src/client/ClientApp.cpp
)
target_link_libraries(client PRIVATE
    Boost::system
	GUILib
)

add_executable(server 
	src/server/main.cpp
	src/server/ClientHTTP.h
)
target_link_libraries(server PRIVATE
    Boost::system
)