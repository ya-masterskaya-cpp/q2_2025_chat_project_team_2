1) Текущая архитектура:
```
irc_char
├── build
├── CMakeLists.txt
├── CMakeUserPresets.json
├── commands4build.txt
├── common
│   ├── add_struct
│   │   └── add_struct.h
│   ├── config
│   │   └── config.h
│   ├── database
│   │   ├── database.cpp
│   │   └── database.h
│   ├── gui
│   │   ├── BBCodeUtils.h
│   │   ├── BBCodeUtils.cpp
│   │   ├── ListSelectionDialog.h
│   │   ├── ListSelectionDialog.cpp
│   │   ├── LoginDialog.cpp
│   │   ├── LoginDialog.h
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   ├── RegisterDialog.h
│   │   └── RegisterDialog.cpp
│   └── ssl
│       ├── ssl.cpp
│       └── ssl.h
├── conanfile.txt
├── src
│   ├── client
│   │   ├── ChatClient.cpp
│   │   ├── ChatClient.h
│   │   ├── ClientApp.cpp
│   │   ├── ClientApp.h
│   │   └── main.cpp
│   └── server
│       ├── ClientApp.h
│       └── main.cpp
├── tree.txt
└── Описание.txt
```
где каталог build - сборка проекта и компилированных библиотек
common - с основными общими для кода клиента и сервера зависимостями, пока тут лежать все что не относится исключительно к коду самого клиента или сервера
common/add_struct - каталог с дополнительными структурами используемыми в проекте, пока там только описана структура message. Так же тут хорошо объявлять глобальные переменные
common/config - каталог с настройками проекта, пока там пустой файл, но там будут находится настройки нашего приложения. Настройки лучше хранить в одном файле
common/database - каталог с библиотекой реализующей работы с базой данных, в нашем коде это sqlite
common/gui - каталог с кодом графического интерфейса нашей программы
common/ssl - каталог с кодом методов отвечающих за шифрование, в нашем случае там лежит метод получения хэша от пароля.
src - каталог с кодом основных запускаемых приложений: клиента и сервера
src/client - каталог с логикой работы клиента
src/server - каталог с логикой работы сервера

3) Теперь рассмотим библиотеки в CMakeLists:
add_struct - интерфейс описывает дополнительные структуры в проекте, а так же конфиг проекта
SSL_Lib - библиотека работы с хешированием пароля
DatabaseLib - библиотек работы с базой данных
ClientLogicLib - библиотека логики работы клиента
GUILib - библиотека отвечающих за графический интерфейс проекта
client - основное запускаемое приложение клиента
