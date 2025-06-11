#include "db.hpp"
#include "stmt.hpp"
#include "time_utils.hpp"
#include <sqlite3.h>
#include <iostream>


namespace sql {
    static const char* GET_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id;
    )sql";
    
    static const char* GET_USERS_ROOM = R"sql(
    SELECT
        u.login,
        u.name,
        u.password_hash,
        r.role,
        u.is_deleted,
        u.unixtime AS user_registration_time
        FROM users u
        JOIN roles r ON u.roles_id = r.roles_id
        JOIN user_rooms ur ON u.users_id = ur.users_id
        JOIN rooms rm ON ur.rooms_id = rm.rooms_id
        WHERE rm.room = ? --Параметр имени комнаты
        AND u.is_deleted = 0;  --Только активные пользователи
    )sql";

    static const char* CREATE_USER = R"sql(
        INSERT OR IGNORE INTO users(login, name, password_hash, roles_id, is_deleted, unixtime)
            VALUES(? , ? , ? , (SELECT roles_id FROM roles WHERE role = ? ), ?, ? );
    )sql";

    static const char* DELETE_USER = R"sql(
        DELETE FROM users 
        WHERE login = ? 
        AND NOT EXISTS (
            SELECT 1 
            FROM user_rooms 
            WHERE user_rooms.users_id = users.users_id
        );
    )sql";

    static const char* DELETE_DELETED_USER_WITHOUT_ROOM = R"sql(
        DELETE FROM users
            WHERE is_deleted = 1
            AND NOT EXISTS(
                SELECT 1
                FROM user_rooms
                WHERE user_rooms.users_id = users.users_id
            );
    )sql";

    static const char* ROOM_USERS_SQL = R"sql(
        SELECT u.login, u.name, u.password_hash, u.role, u.is_deleted
        FROM users u
        JOIN user_rooms ur ON u.users_id = ur.users_id
        JOIN rooms r ON ur.rooms_id = r.rooms_id
        WHERE ur.room = ? ;
    )sql";

    static const char* ADD_USER_TO_ROOM = R"sql(
        INSERT OR IGNORE INTO user_rooms (users_id, rooms_id)
        VALUES (
            (SELECT users_id FROM users WHERE login = ?),
            (SELECT rooms_id FROM rooms WHERE room = ?)
        );
    )sql";

    static const char* DELETE_USER_FROM_ROOM = R"sql(
        DELETE FROM user_rooms
        WHERE users_id = (SELECT users_id FROM users WHERE login = ?)
            AND rooms_id = (SELECT rooms_id FROM rooms WHERE room = ?);
    )sql";

    static const char* GET_RECENT_MESSAGES = R"sql(
        SELECT 
            m.message,
            u.login       AS user_login,
            r.room        AS room_name,
            m.unixtime
        FROM messages AS m
        JOIN users AS u   ON m.users_id = u.users_id
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?
        ORDER BY m.unixtime DESC
        LIMIT 50;
    )sql";

    static const char* GET_MESSAGES_AFTER = R"sql(
        SELECT 
            m.message,
            u.login       AS user_login,
            r.room        AS room_name,
            m.unixtime
        FROM messages AS m
        JOIN users AS u   ON m.users_id = u.users_id
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?
          AND m.unixtime < ?
        ORDER BY m.unixtime DESC
        LIMIT 50;
    )sql";

    static const char* INIT_SQL = R"sql(
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY, 
            value TEXT
        );
        INSERT OR IGNORE INTO metadata (key, value) VALUES ('schema_version', '1');

        CREATE TABLE IF NOT EXISTS roles (
            roles_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            role TEXT UNIQUE NOT NULL
        );
        INSERT OR IGNORE INTO roles(role) VALUES ('admin'), ('user');

        CREATE TABLE IF NOT EXISTS rooms (
            rooms_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            room TEXT UNIQUE NOT NULL,
            unixtime INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_rooms_room ON rooms(room);

        CREATE TABLE IF NOT EXISTS users ( 
            users_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            login TEXT UNIQUE NOT NULL, 
            name TEXT NOT NULL, 
            password_hash TEXT NOT NULL,
            roles_id INTEGER NOT NULL REFERENCES roles(roles_id),
            is_deleted BOOLEAN NOT NULL DEFAULT 0,
            unixtime INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);

        CREATE TABLE IF NOT EXISTS messages (
            messages_id INTEGER PRIMARY KEY AUTOINCREMENT,
            message TEXT NOT NULL,
            unixtime INTEGER NOT NULL,
            users_id INTEGER NOT NULL REFERENCES users(users_id),
            rooms_id INTEGER NOT NULL REFERENCES rooms(rooms_id) ON DELETE CASCADE,
            date TEXT NOT NULL,
            time TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_messages_room_user ON messages(rooms_id, users_id);
        CREATE INDEX IF NOT EXISTS idx_room_time ON messages(rooms_id, unixtime DESC);
        
        CREATE TABLE IF NOT EXISTS user_rooms (
            user_rooms_id INTEGER PRIMARY KEY AUTOINCREMENT,
            users_id INTEGER NOT NULL REFERENCES users(users_id) ON DELETE CASCADE,
            rooms_id INTEGER NOT NULL REFERENCES rooms(rooms_id) ON DELETE CASCADE,
            UNIQUE(users_id, rooms_id)
        );
    )sql";

    static const char* INSERT_TEST_DATA = R"sql(
        -- 1. Создание 10 пользователей
        INSERT INTO users (login, name, password_hash, roles_id, is_deleted, unixtime) VALUES
        ('user_0', 'User Zero', 'hash0', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000000),
        ('user_1', 'User One', 'hash1', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000001),
        ('user_2', 'User Two', 'hash2', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000002),
        ('user_3', 'User Three', 'hash3', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000003),
        ('user_4', 'User Four', 'hash4', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000004),
        ('user_5', 'User Five', 'hash5', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000005),
        ('user_6', 'User Six', 'hash6', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000006),
        ('user_7', 'User Seven', 'hash7', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000007),
        ('user_8', 'User Eight', 'hash8', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000008),
        ('user_9', 'User Nine', 'hash9', (SELECT roles_id FROM roles WHERE role = 'user'), 0, 1000000009);

        -- 2. Создание 10 комнат
        INSERT INTO rooms (room, unixtime) VALUES
        ('room_0', 1000000010),
        ('room_1', 1000000011),
        ('room_2', 1000000012),
        ('room_3', 1000000013),
        ('room_4', 1000000014),
        ('room_5', 1000000015),
        ('room_6', 1000000016),
        ('room_7', 1000000017),
        ('room_8', 1000000018),
        ('room_9', 1000000019);
        -- 3. Размещение пользователей по комнатам
        -- Комнаты с одним пользователем (room_0 и room_1)
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_0'), (SELECT rooms_id FROM rooms WHERE room = 'room_0')),
        ((SELECT users_id FROM users WHERE login = 'user_1'), (SELECT rooms_id FROM rooms WHERE room = 'room_1'));

        -- Комнаты со всеми пользователями (room_2 и room_3)
        INSERT INTO user_rooms (users_id, rooms_id)
        SELECT u.users_id, r.rooms_id
        FROM users u
        CROSS JOIN rooms r
        WHERE r.room IN ('room_2', 'room_3') AND u.login LIKE 'user_%';

        -- Остальные комнаты с хаотичным распределением
        -- room_4: 3 случайных пользователя
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_2'), (SELECT rooms_id FROM rooms WHERE room = 'room_4')),
        ((SELECT users_id FROM users WHERE login = 'user_5'), (SELECT rooms_id FROM rooms WHERE room = 'room_4')),
        ((SELECT users_id FROM users WHERE login = 'user_7'), (SELECT rooms_id FROM rooms WHERE room = 'room_4'));

        -- room_5: 5 случайных пользователей
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_1'), (SELECT rooms_id FROM rooms WHERE room = 'room_5')),
        ((SELECT users_id FROM users WHERE login = 'user_3'), (SELECT rooms_id FROM rooms WHERE room = 'room_5')),
        ((SELECT users_id FROM users WHERE login = 'user_4'), (SELECT rooms_id FROM rooms WHERE room = 'room_5')),
        ((SELECT users_id FROM users WHERE login = 'user_6'), (SELECT rooms_id FROM rooms WHERE room = 'room_5')),
        ((SELECT users_id FROM users WHERE login = 'user_9'), (SELECT rooms_id FROM rooms WHERE room = 'room_5'));

        -- room_6: 4 случайных пользователя
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_0'), (SELECT rooms_id FROM rooms WHERE room = 'room_6')),
        ((SELECT users_id FROM users WHERE login = 'user_2'), (SELECT rooms_id FROM rooms WHERE room = 'room_6')),
        ((SELECT users_id FROM users WHERE login = 'user_4'), (SELECT rooms_id FROM rooms WHERE room = 'room_6')),
        ((SELECT users_id FROM users WHERE login = 'user_8'), (SELECT rooms_id FROM rooms WHERE room = 'room_6'));

        -- room_7: 2 случайных пользователя
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_3'), (SELECT rooms_id FROM rooms WHERE room = 'room_7')),
        ((SELECT users_id FROM users WHERE login = 'user_6'), (SELECT rooms_id FROM rooms WHERE room = 'room_7'));

        -- room_8: 6 случайных пользователей
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_1'), (SELECT rooms_id FROM rooms WHERE room = 'room_8')),
        ((SELECT users_id FROM users WHERE login = 'user_2'), (SELECT rooms_id FROM rooms WHERE room = 'room_8')),
        ((SELECT users_id FROM users WHERE login = 'user_3'), (SELECT rooms_id FROM rooms WHERE room = 'room_8')),
        ((SELECT users_id FROM users WHERE login = 'user_5'), (SELECT rooms_id FROM rooms WHERE room = 'room_8')),
        ((SELECT users_id FROM users WHERE login = 'user_7'), (SELECT rooms_id FROM rooms WHERE room = 'room_8')),
        ((SELECT users_id FROM users WHERE login = 'user_9'), (SELECT rooms_id FROM rooms WHERE room = 'room_8'));

        -- room_9: 7 случайных пользователей
        INSERT INTO user_rooms (users_id, rooms_id) VALUES
        ((SELECT users_id FROM users WHERE login = 'user_0'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_1'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_3'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_4'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_6'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_7'), (SELECT rooms_id FROM rooms WHERE room = 'room_9')),
        ((SELECT users_id FROM users WHERE login = 'user_8'), (SELECT rooms_id FROM rooms WHERE room = 'room_9'));

        -- 4. Генерация сообщений в 50% комнат (room_0, room_2, room_4, room_6, room_8)
        -- room_0: 10 сообщений (только user_0)
        WITH RECURSIVE cnt(x) AS (
            SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<10
        )
        INSERT INTO messages (message, unixtime, users_id, rooms_id, date, time)
        SELECT 
            'Message ' || x || ' in room_0 by user_0',
            1100000000 + x,
            (SELECT users_id FROM users WHERE login = 'user_0'),
            (SELECT rooms_id FROM rooms WHERE room = 'room_0'),
            date(1100000000 + x, 'unixepoch'),
            time(1100000000 + x, 'unixepoch')
        FROM cnt;

        -- room_2: 20 сообщений (от разных пользователей)
        WITH RECURSIVE cnt(x) AS (
            SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<20
        )
        INSERT INTO messages (message, unixtime, users_id, rooms_id, date, time)
        SELECT 
            'Message ' || x || ' in room_2 by user_' || (x % 10),
            1200000000 + x,
            (SELECT users_id FROM users WHERE login = 'user_' || (x % 10)),
            (SELECT rooms_id FROM rooms WHERE room = 'room_2'),
            date(1200000000 + x, 'unixepoch'),
            time(1200000000 + x, 'unixepoch')
        FROM cnt;

        -- room_4: 30 сообщений (от пользователей в этой комнате: user_2, user_5, user_7)
        WITH RECURSIVE cnt(x) AS (
            SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<30
        )
        INSERT INTO messages (message, unixtime, users_id, rooms_id, date, time)
        SELECT 
            'Message ' || x || ' in room_4 by user_' || CASE WHEN x % 3 = 0 THEN '7' WHEN x % 3 = 1 THEN '2' ELSE '5' END,
            1300000000 + x,
            (SELECT users_id FROM users WHERE login = CASE WHEN x % 3 = 0 THEN 'user_7' WHEN x % 3 = 1 THEN 'user_2' ELSE 'user_5' END),
            (SELECT rooms_id FROM rooms WHERE room = 'room_4'),
            date(1300000000 + x, 'unixepoch'),
            time(1300000000 + x, 'unixepoch')
        FROM cnt;

        -- room_6: 40 сообщений (от пользователей в этой комнате: user_0, user_2, user_4, user_8)
        WITH RECURSIVE cnt(x) AS (
            SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<40
        )
        INSERT INTO messages (message, unixtime, users_id, rooms_id, date, time)
        SELECT 
            'Message ' || x || ' in room_6 by user_' || CASE WHEN x % 4 = 0 THEN '0' WHEN x % 4 = 1 THEN '2' WHEN x % 4 = 2 THEN '4' ELSE '8' END,
            1400000000 + x,
            (SELECT users_id FROM users WHERE login = CASE WHEN x % 4 = 0 THEN 'user_0' WHEN x % 4 = 1 THEN 'user_2' WHEN x % 4 = 2 THEN 'user_4' ELSE 'user_8' END),
            (SELECT rooms_id FROM rooms WHERE room = 'room_6'),
            date(1400000000 + x, 'unixepoch'),
            time(1400000000 + x, 'unixepoch')
        FROM cnt;

        -- room_8: 55 сообщений (от пользователей в этой комнате: user_1, user_2, user_3, user_5, user_7, user_9)
        WITH RECURSIVE cnt(x) AS (
            SELECT 1 UNION ALL SELECT x+1 FROM cnt WHERE x<55
        )
        INSERT INTO messages (message, unixtime, users_id, rooms_id, date, time)
        SELECT 
            'Message ' || x || ' in room_8 by user_' || CASE 
                WHEN x % 6 = 0 THEN '1' 
                WHEN x % 6 = 1 THEN '2' 
                WHEN x % 6 = 2 THEN '3' 
                WHEN x % 6 = 3 THEN '5' 
                WHEN x % 6 = 4 THEN '7' 
                ELSE '9' END,
            1500000000 + x,
            (SELECT users_id FROM users WHERE login = CASE 
                WHEN x % 6 = 0 THEN 'user_1' 
                WHEN x % 6 = 1 THEN 'user_2' 
                WHEN x % 6 = 2 THEN 'user_3' 
                WHEN x % 6 = 3 THEN 'user_5' 
                WHEN x % 6 = 4 THEN 'user_7' 
                ELSE 'user_9' END),
            (SELECT rooms_id FROM rooms WHERE room = 'room_8'),
            date(1500000000 + x, 'unixepoch'),
            time(1500000000 + x, 'unixepoch')
        FROM cnt;

    )sql";

}

DB::DB() {}
DB::DB(const std::string& db_file) : db_filename_(db_file), db_(nullptr) {}

DB::~DB() {
    CloseDB();
}

bool DB::DeleteDBFile() {
    CloseDB();  // Закрываем соединение перед удалением
    errno = 0;
    bool result = std::remove(db_filename_.c_str()) == 0;
    return result;
}

bool DB::RecreateDB() {
    try {
        // 1. Удаляем файл БД (если он существует)
        if (!DeleteDBFile()) {
            // Если файла нет, это не ошибка (может быть первый запуск)
            if (errno != ENOENT) {  // ENOENT = "No such file or directory"
                throw std::runtime_error("Failed to delete DB file");
            }
        }

        // 2. Открываем новое соединение (файл создастся автоматически)
        if (!OpenDB()) {
            throw std::runtime_error("Failed to reopen DB");
        }

        // 3. Инициализируем схему (INIT_SQL)
        if (!InitSchema()) {
            throw std::runtime_error("Failed to initialize DB schema");
        }

        return true;
    }
    catch (const std::exception& e) {
        // Логируем ошибку (если есть логгер)
        std::cerr << "DB::RecreateDB error: " << e.what() << std::endl;
        return false;
    }
}

bool DB::InsertTestDataToBD() {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql::INSERT_TEST_DATA, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[InsertTestDataToBD] SQL error: " << errmsg << "\n";
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool CheckVersionDB() {
    /*
        нужно реализовать проверку версионности
    */
    return true;
}

bool DB::OpenDB() {
    if (db_) {
        return true; // уже открыта
    }

    if (sqlite3_open(db_filename_.c_str(), &db_) != SQLITE_OK) {
        return false;
    }
    // Включаем Write-Ahead Logging (WAL), безопасно и ускоряет запись (без потери данных)
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // Баланс между безопасностью и скоростью. Полная гарантия сохранения даже при сбое — FULL, но NORMAL приемлемо
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    // Включаем поддержку внешних ключей сразу после открытия
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    if (!CheckVersionDB()) {
        std::cerr << "Incompatible DB schema version.\n";
        return false;
    }

    return InitSchema();
}

void DB::CloseDB() {
    if (db_) {
        sqlite3_close(reinterpret_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
}


bool DB::CreateRoom(const std::string& room, int64_t unixtime) {
    Stmt stmt(db_, "INSERT OR IGNORE INTO rooms (room, unixtime) VALUES (?, ?);");
    stmt.Bind(1, room);
    stmt.Bind(2, unixtime);
    bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
    return success;
}

bool DB::DeleteRoom(const std::string& room) {
    Stmt stmt(db_, "DELETE FROM rooms WHERE room = ?;");
    stmt.Bind(1, room);
    bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;

    DelDeletedUsersWithoutRoom(); // при каждом удалении комнаты проверяем возможность окончательного удаления мягко удаленных пользователей

    return success;
}

bool DB::IsRoom(const std::string& room) {
    Stmt stmt(db_, "SELECT EXISTS (SELECT 1 FROM rooms WHERE room = ?);"); 
    stmt.Bind(1, room);
    if (sqlite3_step(stmt.Get()) != SQLITE_ROW) {
        std::cerr << "[IsRoom] SQL error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }
    return sqlite3_column_int(stmt.Get(), 0) != 0;
}

std::vector<std::string> DB::GetRooms() {
    Stmt stmt(db_, "SELECT room FROM rooms;");
    std::vector < std::string> result;
    while (sqlite3_step(stmt.Get()) == SQLITE_ROW) {
        result.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt.Get(), 0)));
    }
    return result;
}

bool DB::CreateUser(const User& user) {
    Stmt stmt(db_, sql::CREATE_USER);
    stmt.Bind(1, user.login);
    stmt.Bind(2, user.name);
    stmt.Bind(3, user.password_hash);
    stmt.Bind(4, user.role);
    stmt.Bind(5, user.is_deleted);
    stmt.Bind(6, user.unixtime);
    bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
    return success;
}

bool DB::SetUserForDelete(const std::string& user_login) {
    Stmt stmt(db_, "UPDATE users SET is_deleted = 1 WHERE login = ?;");
    stmt.Bind(1, user_login);
    bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
    return success;
}

bool DB::DeleteUser(const std::string& user_login) {

    bool success1 = SetUserForDelete(user_login);

    Stmt stmt(db_, sql::DELETE_USER);
    stmt.Bind(1, user_login);
    bool success2 = sqlite3_step(stmt.Get()) == SQLITE_DONE;

    return success1 && success2;
}

bool DB::IsUser(const std::string& user_login) {
    Stmt stmt(db_, "SELECT EXISTS (SELECT 1 FROM users WHERE login = ?);");
    stmt.Bind(1, user_login);
    if (sqlite3_step(stmt.Get()) != SQLITE_ROW) {
        std::cerr << "[IsUser] SQL error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }
    return sqlite3_column_int(stmt.Get(), 0) != 0;
}

std::vector<User> DB::GetUsers() {
    std::vector<User> users;
    Stmt stmt(db_, sql::GET_USERS);
    while (sqlite3_step(stmt.Get()) == SQLITE_ROW) {
        std::string login = stmt.GetColumnText(stmt.Get(), 0);
        std::string name = stmt.GetColumnText(stmt.Get(), 1);
        std::string password_hash = stmt.GetColumnText(stmt.Get(), 2);
        std::string role = stmt.GetColumnText(stmt.Get(), 3);
        bool is_deleted = sqlite3_column_int(stmt.Get(), 4) != 0;
        int64_t unixtime = sqlite3_column_int(stmt.Get(), 5);
        users.emplace_back(login, name, password_hash, role, is_deleted, unixtime);
    }
    return users;
}

std::vector<User> DB::GetRoomUsers(const std::string& room) {
    std::vector<User> users;
    Stmt stmt(db_, sql::GET_USERS_ROOM);
    stmt.Bind(1, room);
    while (sqlite3_step(stmt.Get()) == SQLITE_ROW) {
        std::string login = stmt.GetColumnText(stmt.Get(), 0);
        std::string name = stmt.GetColumnText(stmt.Get(), 1);
        std::string password_hash = stmt.GetColumnText(stmt.Get(), 2);
        std::string role = stmt.GetColumnText(stmt.Get(), 3);
        bool is_deleted = sqlite3_column_int(stmt.Get(), 4) != 0;
        int64_t unixtime = sqlite3_column_int(stmt.Get(), 5);
        users.emplace_back(login, name, password_hash, role, is_deleted, unixtime);
    }
    return users;
}

bool DB::PerformSQLReturnBool(const char* sql_query, std::vector<std::string> param) {
    Stmt stmt(db_, sql_query);
    for (size_t i = 0; i < param.size(); ++i) {
        stmt.Bind(static_cast<int>(i + 1), param[i]);
    }
    bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
    return success;
}

bool DB::AddUserToRoom(const std::string& user_login, const std::string& room) {
    return PerformSQLReturnBool(sql::ADD_USER_TO_ROOM, { user_login , room });
}

bool DB::DeleteUserFromRoom(const std::string& user_login, const std::string& room) {
    return PerformSQLReturnBool(sql::DELETE_USER_FROM_ROOM, { user_login , room });
}

bool DB::InitSchema() {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(reinterpret_cast<sqlite3*>(db_), sql::INIT_SQL, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to initialize DB schema: " << errmsg << "\n";
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool DB::DelDeletedUsersWithoutRoom() {
    Stmt stmt(db_, sql::DELETE_DELETED_USER_WITHOUT_ROOM);
    return sqlite3_step(stmt.Get()) == SQLITE_DONE;
}
