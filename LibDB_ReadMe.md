## sqlite_bd

Минималистичная библиотека-обёртка для работы с `SQLite3` на `C++17`. Кроссплатформенность: Windows и Linux
`Conan` + `CMake`, автоматические тесты `Catch2`

### Быстрая сборка (MS Visual Studio)

```bash
rm -rf build
conan install . --output-folder=build --build=missing -s build_type=Debug
# сборка без тестовой части с ключем -DBUILD_TESTING=OFF
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build --config Debug
```
### Структура проекта
<pre>
libdb/
├── include/        
├── src/            
├── CMakeLists.txt  
├── conanfile.txt 
└── test/           </pre>

### Используемые структуры:
```cpp
struct User {
    std::string login;
    std::string name;
    std::string password_hash;
    std::string role;
    bool is_deleted;
    int64_t unixtime; //наносекунды
};
struct Message {
    std::string message;
    int64_t unixtime; //наносекунды
    std::string user_login;
    std::string room;
};
```
При отсутствии файла БД, будет создан пустой файл БД. </br>
Униальные поля: логин пользователя, наименование комнаты и роль пользователя в БД. </br>
### Перечень функций работы с БД

#### 1. Управление подключением
- `OpenDB()` – открывает соединение с БД, инициализирует схему (если БД новая).
``` cpp
      bool OpenDB();
```
- `CloseDB()` – закрывает соединение.
``` cpp
      void CloseDB();
```
- `GetVersionDB()` - возвращает внутренний номер версии БД.
``` cpp
      td::string GetVersionDB();
```

#### 2. Управление пользователями
- `CreateUser(user)` – добавляет пользователя.
``` cpp
      bool CreateUser(const User& user);
```
- `DeleteUser(login)` – удаляет пользователя, если он не состоит в комнатах (иначе только is_deleted = true).
``` cpp
      bool DeleteUser(const std::string& user_login);
```
- `IsUser(login)` – проверяет существование пользователя.
``` cpp
      bool IsUser(const std::string& user_login);
```
- `IsAliveUser(login)` – проверяет, что пользователь существует и не помечен на удаление.
``` cpp
      bool IsAliveUser(const std::string& user_login);
```
- `GetAllUsers()` – возвращает всех пользователей (включая удаленных).
``` cpp
      std::vector<User> GetAllUsers();
```
- `GetActiveUsers()` – возвращает только активных (is_deleted = false).
``` cpp
      std::vector<User> GetActiveUsers();
```
- `GetDeletedUsers()` – возвращает удаленных (is_deleted = true).
``` cpp
      std::vector<User> GetDeletedUsers();
```
#### 3. Управление комнатами
- `CreateRoom(room, unixtime)` – создает комнату.
``` cpp
      bool CreateRoom(const std::string& room, int64_t unixtime);
```
- `DeleteRoom(room)` – удаляет комнату, сообщения комнаты из БД и связи пользователей с конатой.
``` cpp
      bool DeleteRoom(const std::string& room);
```
- `IsRoom(room)` – проверяет существование комнаты.
``` cpp
      bool IsRoom(const std::string& room);
```
- `GetRooms()` – возвращает список всех комнат.
``` cpp
      std::vector<std::string> GetRooms();
```

#### 4. Связи пользователей и комнат
- `AddUserToRoom(login, room)` – добавляет пользователя в комнату.
``` cpp
      bool AddUserToRoom(const std::string& user_login, const std::string& room);
```
- `DeleteUserFromRoom(login, room)` – удаляет пользователя из комнаты.
``` cpp
      bool DeleteUserFromRoom(const std::string& user_login, const std::string& room);
```
- `GetUserRooms(login)` – возвращает комнаты пользователя.
``` cpp
      std::vector<std::string> GetUserRooms(const std::string& user_login);
```
- `GetRoomActiveUsers(room)` – возвращает активных пользователей комнаты.
``` cpp
      std::vector<User> GetRoomActiveUsers(const std::string& room);
```

#### 5. Управление сообщениями
- `InsertMessageToDB(message)` – добавляет сообщение в комнату.
``` cpp
      bool InsertMessageToDB(const Message& message);
```
- `GetRecentMessagesRoom(room)` – возвращает 50 последних сообщений.
``` cpp
      std::vector<Message> GetRecentMessagesRoom(const std::string& room);
```
- `GetMessagesRoomAfter(room, unixtime)` – возвращает 50 сообщений, начиная с указанного времени (пагинация).
``` cpp
      std::vector<Message> GetMessagesRoomAfter(const std::string& room, int64_t unixtime);
```
- `GetCountRoomMessages(room)` – возвращает количество сообщений в комнате.
``` cpp
      int GetCountRoomMessages(const std::string& room);
```

### Структура таблиц БД

#### `ТАБЛИЦА metadata` </br>
- `key` (TEXT, PRIMARY KEY) – ключ (например, schema_version).</br>
- `value` (TEXT) – значение.</br>

#### `ТАБЛИЦА roles`
- `roles_id` (INTEGER, PRIMARY KEY) – ID роли.
- `role` (TEXT, UNIQUE) – название роли (admin, user).

#### `ТАБЛИЦА rooms`
- `rooms_id` (INTEGER, PRIMARY KEY) – ID комнаты.
- `room` (TEXT, UNIQUE) – название комнаты.
- `unixtime` (INTEGER) – время создания (наносекунды).

#### `ТАБЛИЦА users`
- `users_id` (INTEGER, PRIMARY KEY) – ID пользователя.
- `login` (TEXT, UNIQUE) – логин.
- `name` (TEXT) – имя.
- `password_hash` (TEXT) – хеш пароля.
- `roles_id` (INTEGER, FOREIGN KEY) – ссылка на roles.roles_id.
- `is_deleted` (BOOLEAN) – флаг удаления.
- `unixtime` (INTEGER) – время регистрации (наносекунды).

#### `ТАБЛИЦА messages`
- `messages_id` (INTEGER, PRIMARY KEY) – ID сообщения.
- `message` (TEXT) – текст.
- `unixtime` (INTEGER) – время отправки (наносекунды).
- `users_id` (INTEGER, FOREIGN KEY) – отправитель (users.users_id).
- `rooms_id` (INTEGER, FOREIGN KEY) – комната (rooms.rooms_id, ON DELETE CASCADE).
- `date` (TEXT) – дата в формате YYYY-MM-DD.
- `time` (TEXT) – время в формате HH:MM:SS.

#### `ТАБЛИЦА user_rooms`
- `user_rooms_id` (INTEGER, PRIMARY KEY) – ID связи.
- `users_id` (INTEGER, FOREIGN KEY, ON DELETE CASCADE) – пользователь.
- `rooms_id` (INTEGER, FOREIGN KEY, ON DELETE CASCADE) – комната.

UNIQUE(users_id, rooms_id) – запрет дублирования связей.

### Ключевые зависимости

Каскадное удаление: </br>
Удаление комнаты → автоматически удаляет записи в user_rooms и messages.
