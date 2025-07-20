## IRC-Chat Project
Это клиент-серверное чат-приложение, разработанное на C++. Серверная часть использует Boost.Asio и Boost.Beast для реализации многопоточного WebSocket-сервера, а для хранения данных (пользователи, комнаты, история сообщений) применяется SQLite. Клиент представляет собой десктопное приложение с графическим интерфейсом, созданным с помощью wxWidgets.

Проект демонстрирует полный цикл разработки: от сетевого взаимодействия и работы с базой данных на сервере до реализации пользовательского интерфейса на клиенте.

### Команды сборки для Windows из корневой папки проекта
- Для Debug:
```
conan install . --output-folder=build/debug -s build_type=Debug --build=missing
cmake -S . -B build/debug -G "Visual Studio 17 2022" -A x64
cmake --build build/debug --config Debug
```
- Для Release:
```
conan install conanfile_release.txt --output-folder=build/release -s build_type=Release --build=missing
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
```
## Структура проекта
Проект организован по модульному принципу для разделения ответственностей между компонентами.
- CMakeLists.txt: Главный файл сборки, который управляет всем проектом.
- conanfile.txt: Файлы конфигурации менеджера пакетов Conan, определяют все внешние зависимости (Boost, wxWidgets, SQLite и др.).
- build/: Директория для сгенерированных файлов сборки (не отслеживается Git).
- resources/: Иконки и другие ресурсы приложения.
### src/ - Основной исходный код
#### src/client/: Содержит основную логику клиентского приложения.
- ClientApp.h/cpp: Класс приложения wxWidgets, точка входа в GUI.
- ChatClient.h/cpp: Слой бизнес-логики клиента, связывающий GUI с сетевой частью.
- ClientHTTP.h: Низкоуровневый сетевой клиент, управляющий WebSocket-соединениями (Boost.Beast).
#### src/server/: Содержит точку входа (main.cpp) для исполняемого файла сервера. Основная логика вынесена в библиотеку.
### common/ - Переиспользуемые библиотеки и модули
- common/chat_server_lib/: Статическая библиотека, содержащая всю основную логику сервера (Server.h/cpp).
- common/libdb/: Статическая библиотека для взаимодействия с базой данных SQLite. Содержит SQL-запросы и обертки для работы с БД.
- common/gui/: Содержит все классы, отвечающие за графический интерфейс пользователя (GUI) на wxWidgets (MainWindow, LoginDialog и др.).
- common/add_struct/: Общие структуры данных (MsgQueue, MesBuilder) и константы (General.h), используемые как клиентом, так и сервером.
- common/config/: Файлы конфигурации (порты, адреса и т.д.).
- common/ssl/: Утилиты для работы с хэшированием (например, для паролей).

[Описание библиотеки БД](./common/libdb/LibDB_ReadMe.md)

[Описание интеграции сервера и БД](./common/chat_server_lib/Server_DB_integration.md)

### Архитектура проекта:
```
irc_char
├── CMakeLists.txt
├── CMakeUserPresets.json
├── commands4build.txt
├── conanfile.txt
├── conanfile_release.txt
│
├── common
│   ├── add_struct
│   │   ├── General.h
│   │   ├── add_struct.h
│   │   └── common_struct.h
│   │
│   ├── chat_server_lib
│   │   ├── include
│   │   │   ├── Logger.h
│   │   │   └── Server.h
│   │   ├── src
│   │   │   ├── Server.cpp
│   │   │   └── json.h
│   │   ├── CMakeLists.txt
│   │   └── Server_DB_integration.md
│   │
│   ├── config
│   │   └── config.h
│   │
│   ├── gui
│   │   ├── BBCodeUtils.h
│   │   ├── BBCodeUtils.cpp
│   │   ├── ConfigManager.cpp
│   │   ├── ConfigManager.h
│   │   ├── ListSelectionDialog.h
│   │   ├── ListSelectionDialog.cpp
│   │   ├── LoginDialog.cpp
│   │   ├── LoginDialog.h
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   ├── RegisterDialog.h
│   │   └── RegisterDialog.cpp
│   │
│   ├── libdb
│   │   ├── build
│   │   │   └── .gitkeep
│   │   ├── include
│   │   │   ├── db.hpp
│   │   │   └── time_utils.hpp
│   │   ├── src
│   │   │   ├── db.cpp
│   │   │   ├── sql_queries.hpp
│   │   │   └── stmt.hpp
│   │   ├── test
│   │   │   ├── main.cppcopy
│   │   │   └── test.cpp
│   │   ├── CMakeLists.txt
│   │   ├── LibDB_ReadMe.md
│   │   └── conanfile.txt
│   │
│   └── ssl
│       ├── ssl.cpp
│       └── ssl.h
│   │
├── resources
│   ├── icon.icns
│   ├── icon.ico
│   └── icon.png
│   │
├── src
│   ├── client
│   │   ├── ChatClient.cpp
│   │   ├── ChatClient.h
│   │   ├── ClientApp.cpp
│   │   ├── ClientApp.h
│   │   ├── ClientHTTP.h
│   │   └── main.cpp
│   │
│   └── server
│       ├── README.md
│       └── main.cpp
└── Описание.txt
```
