# Корневой CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

project(irc_chat CXX)
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  # Устанавливаем минимальную целевую версию Windows.
  # 0x0A00 - Windows 10.
  # 0x0601 - Windows 7.
  add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# Подключение Conan
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake) # Генерируется CMakeToolchain

# Поиск зависимостей
find_package(Boost REQUIRED COMPONENTS 
    system 
    beast 
    CONFIG
)
find_package(wxWidgets REQUIRED COMPONENTS core base adv rich CONFIG)
find_package(SQLite3 REQUIRED CONFIG)
find_package(OpenSSL REQUIRED CONFIG) 
find_package(nlohmann_json REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(Catch2 REQUIRED CONFIG)

# Настройка включения ресурсов
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(ICON_WIN "${RESOURCE_DIR}/icon.ico")
set(ICON_LINUX "${RESOURCE_DIR}/icon.png")
set(ICON_MACOS "${RESOURCE_DIR}/icon.icns")

# Проверим существование файлов перед добавлением
if(EXISTS ${ICON_WIN})
    message(STATUS "Windows icon found: ${ICON_WIN}")
else()
    message(WARNING "Windows icon not found: ${ICON_WIN}")
endif()

# Создаем .rc файл для Windows
if(WIN32)
    # Создаем .rc файл в бинарной директории
    set(WIN_RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    file(WRITE ${WIN_RESOURCE_FILE} "APP_ICON ICON \"${ICON_WIN}\"\n")
    set_source_files_properties(${WIN_RESOURCE_FILE} PROPERTIES LANGUAGE RC)
    
    # Копируем иконку в бинарную директорию
    configure_file(${ICON_WIN} ${CMAKE_CURRENT_BINARY_DIR}/icon.ico COPYONLY)
endif()

# для работы с базой данных
add_subdirectory(common/chat_server_lib)
add_subdirectory(common/libdb)

# дополнительные структуры
add_library(add_struct INTERFACE 
	common/add_struct/add_struct.h
	common/config/config.h
	common/add_struct/General.h
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

# для работы с client_logic
add_library(ClientLogicLib STATIC 
	src/client/ChatClient.h
	src/client/ChatClient.cpp
	src/client/ClientHTTP.h
)
target_include_directories(ClientLogicLib PUBLIC 
    ${PROJECT_SOURCE_DIR}/src/client
)
target_link_libraries(ClientLogicLib PUBLIC 
	SSL_Lib
	add_struct
	Boost::boost
	nlohmann_json::nlohmann_json
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
	wxWidgets::wxWidgets 
	ClientLogicLib
)

# Создание цели
add_executable(client 
	src/client/main.cpp
	src/client/ClientApp.h
	src/client/ClientApp.cpp
	${WIN_RESOURCE_FILE}
    ${ICON_WIN}
    ${ICON_LINUX}
    ${ICON_MACOS}
)
# Копирование ресурсов с проверкой существования
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:client>/resources
    COMMAND ${CMAKE_COMMAND} -E copy
        "${RESOURCE_DIR}/icon.png"
        $<TARGET_FILE_DIR:client>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy
        "${RESOURCE_DIR}/icon.ico"
        $<TARGET_FILE_DIR:client>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy
        "${RESOURCE_DIR}/icon.icns"
        $<TARGET_FILE_DIR:client>/resources/
)
target_link_libraries(client PRIVATE
    Boost::system
	GUILib
)

add_executable(server 
	src/server/main.cpp
)
target_link_libraries(server PRIVATE ChatServerLib)


# Настройка иконки для macOS
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE icon.icns
    )
    set_source_files_properties(${ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()