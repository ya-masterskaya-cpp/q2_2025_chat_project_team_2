# Структура проекта

libdb/
├── include/
│   └── db.hpp
├── src/
│   └── db.cpp
│   └── stmt.hpp
│   └── time_utils.hpp
├── sqlite3/
│   ├── sqlite3.c
│   └── sqlite3.h
├── CMakeLists.txt
└── test/
    └── main.cpp

# Команды сборки для MS VS:

cd libdb
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
