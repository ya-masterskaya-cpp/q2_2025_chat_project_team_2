#include "db.hpp"
#include "stmt.hpp"
#include "time_utils.hpp"
#include <sqlite3.h>
#include <iostream>

namespace sql {
    static const char* GET_ALL_USERS = R"sql(
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

    static const char* GET_ACTIVE_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id
        WHERE u.is_deleted = false;
    )sql";

    static const char* GET_DELETED_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id
        WHERE u.is_deleted = true;
    )sql";

    static const char* GET_ROOM_ACTIVE_USERS = R"sql(
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

    static const char* GET_USER_ROOMS = R"sql(
    SELECT
        r.room
        FROM rooms r
        JOIN user_rooms ur ON r.rooms_id = ur.rooms_id
        JOIN users u ON u.users_id = ur.users_id
        WHERE u.login = ?;
    )sql";

    static const char* CREATE_USER = R"sql(
        INSERT OR IGNORE INTO users(login, name, password_hash, roles_id, is_deleted, unixtime)
            VALUES(? , ? , ? , (SELECT roles_id FROM roles WHERE role = ? ), ?, ? );
    )sql";

    static const char* DELETE_USER = R"sql(
        DELETE FROM users AS u
        WHERE u.login = ?
        AND NOT EXISTS (
            SELECT 1
            FROM user_rooms AS ur
            WHERE ur.users_id = u.users_id
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
        SELECT
            u.login,
            u.name,
            u.password_hash,
            ro.role,
            u.is_deleted
        FROM users AS u
        JOIN user_rooms AS ur ON u.users_id = ur.users_id
        JOIN rooms      AS r  ON ur.rooms_id = r.rooms_id
        JOIN roles      AS ro ON u.roles_id = ro.roles_id -- Добавляем JOIN с roles
        WHERE r.room = ?;
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

    static const char* GET_RECENT_ROOM_MESSAGES = R"sql(
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

    static const char* GET_MESSAGES_ROOM_AFTER = R"sql(
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

    static const char* GET_COUNT_ROOM_MESSAGES = R"sql(
        SELECT COUNT(messages_id)
        FROM messages AS m
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?;
    )sql";

    static const char* INSERT_MESSAGE_TO_DB = R"sql(
        INSERT INTO messages(
            message,
            unixtime,
            users_id,
            rooms_id,
            date,
            time
        )
            VALUES(
                ?, ?,
                (SELECT users_id FROM users WHERE login = ?),
                (SELECT rooms_id FROM rooms WHERE room = ?),
                ?, ?
            );
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

} // sql

namespace db {
    DB::DB() {}
    DB::DB(const std::string& db_file) : db_filename_(db_file), db_(nullptr) {}

    DB::~DB() {
        CloseDB();
    }

    std::string DB::GetVersionDB() {
        std::string result;
        Stmt stmt(db_, "SELECT key, value FROM metadata LIMIT 1;"); // принимаем, что имеется только одна строка в таблице
        if (sqlite3_step(stmt.Get()) != SQLITE_ROW) {
            std::cerr << "[IsUser] SQL error: " << sqlite3_errmsg(db_) << "\n";
            return result;
        }
        std::string key = stmt.GetColumnText(0);
        std::string value = stmt.GetColumnText(1);
        return value;
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
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            result.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt.Get(), 0)));
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
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

    // пользователь удалится, если ни в одной комнате он не зарегистрирован, иначе SetUserForDelete
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

    bool DB::IsAliveUser(const std::string& user_login) {
        Stmt stmt(db_, "SELECT EXISTS (SELECT 1 FROM users WHERE login = ? AND is_deleted = false);");
        stmt.Bind(1, user_login);
        if (sqlite3_step(stmt.Get()) != SQLITE_ROW) {
            std::cerr << "[IsAliveUser] SQL error: " << sqlite3_errmsg(db_) << "\n";
            return false;
        }
        return sqlite3_column_int(stmt.Get(), 0) != 0;
    }

    std::vector<User> DB::GetUsers(const char* sql) {
        std::vector<User> users;
        Stmt stmt(db_, sql);
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            std::string login = stmt.GetColumnText(0);
            std::string name = stmt.GetColumnText(1);
            std::string password_hash = stmt.GetColumnText(2);
            std::string role = stmt.GetColumnText(3);
            bool is_deleted = sqlite3_column_int(stmt.Get(), 4) != 0;
            int64_t unixtime = sqlite3_column_int64(stmt.Get(), 5);
            users.emplace_back(login, name, password_hash, role, is_deleted, unixtime);
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
        }
        return users;
    }

    std::vector<User> DB::GetAllUsers() {
        return GetUsers(sql::GET_ALL_USERS);
    }

    std::vector<User> DB::GetActiveUsers() {
        return GetUsers(sql::GET_ACTIVE_USERS);
    }

    std::vector<User> DB::GetDeletedUsers() {
        return GetUsers(sql::GET_DELETED_USERS);
    }

    std::vector<std::string> DB::GetUserRooms(const std::string& user_login) {
        std::vector<std::string> result;
        Stmt stmt(db_, sql::GET_USER_ROOMS);
        stmt.Bind(1, user_login);
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            result.emplace_back(stmt.GetColumnText(0));
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
        }
        return result;
    }

    std::vector<User> DB::GetRoomActiveUsers(const std::string& room) {
        std::vector<User> users;
        Stmt stmt(db_, sql::GET_ROOM_ACTIVE_USERS);
        stmt.Bind(1, room);
        while (sqlite3_step(stmt.Get()) == SQLITE_ROW) {
            std::string login = stmt.GetColumnText(0);
            std::string name = stmt.GetColumnText(1);
            std::string password_hash = stmt.GetColumnText(2);
            std::string role = stmt.GetColumnText(3);
            bool is_deleted = sqlite3_column_int(stmt.Get(), 4) != 0;
            int64_t unixtime = sqlite3_column_int64(stmt.Get(), 5);
            users.emplace_back(login, name, password_hash, role, is_deleted, unixtime);
        }
        return users;
    }

    std::vector<Message> DB::GetRecentMessagesRoom(const std::string& room) {
        std::vector<Message> messages;
        Stmt stmt(db_, sql::GET_RECENT_ROOM_MESSAGES);
        stmt.Bind(1, room);
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            std::string message = stmt.GetColumnText(0);
            std::string login = stmt.GetColumnText(1);
            std::string room = stmt.GetColumnText(2);
            int64_t unixtime = sqlite3_column_int64(stmt.Get(), 3);
            messages.emplace_back(message, unixtime, login, room);
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
        }
        return messages;
    }

    std::vector<Message> DB::GetMessagesRoomAfter(const std::string& room, int64_t unixtime) {
        std::vector<Message> messages;
        Stmt stmt(db_, sql::GET_MESSAGES_ROOM_AFTER);
        stmt.Bind(1, room);
        stmt.Bind(2, unixtime);
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            std::string message = stmt.GetColumnText(0);
            std::string login = stmt.GetColumnText(1);
            std::string room = stmt.GetColumnText(2);
            int64_t unixtime = sqlite3_column_int64(stmt.Get(), 3);
            messages.emplace_back(message, unixtime, login, room);
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
        }
        return messages;
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

    bool DB::InsertMessageToDB(const Message& message) {
        auto [date,time] = utime::UnixTimeToDateTime(message.unixtime);
        Stmt stmt(db_, sql::INSERT_MESSAGE_TO_DB);
        stmt.Bind(1, message.message);
        stmt.Bind(2, message.unixtime);
        stmt.Bind(3, message.user_login);
        stmt.Bind(4, message.room);
        stmt.Bind(5, date);
        stmt.Bind(6, time);
        bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
        return success;
    }

    int DB::GetCountRoomMessages(const std::string& room) {
        Stmt stmt(db_, sql::GET_COUNT_ROOM_MESSAGES);
        stmt.Bind(1, room);
        int rc = sqlite3_step(stmt.Get());

        if (rc == SQLITE_ROW) {
            return sqlite3_column_int(stmt.Get(), 0);
        }
        std::cerr << "[GetCountRoomMessages] SQL error or unexpected result (" << rc << "): "
            << sqlite3_errmsg(db_) << "\n";
        return -1;
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
} // db

