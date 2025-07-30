#include <iostream>
#include <sqlite3.h>

#include "db.hpp"
#include "sql_queries.hpp"
#include "stmt.hpp"
#include "time_utils.hpp"

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
            заглушка
        */
        return true;
    }

    bool DB::OpenDB() {
        if (db_) {
            return true;
        }

        if (sqlite3_open(db_filename_.c_str(), &db_) != SQLITE_OK) {
            return false;
        }
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
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

        DelDeletedUsersWithoutRoom();

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

    bool DB::ChangeUserName(const std::string& user_login, const std::string& new_name) {
        Stmt stmt(db_, sql::CHANGE_USER_NAME);
        stmt.Bind(1, new_name);
        stmt.Bind(2, user_login);
        bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
        return success;
    }

    bool DB::ChangeRoomName(const std::string& current_room_name, const std::string& new_room_name) {
        Stmt stmt(db_, sql::CHANGE_ROOM_NAME);
        stmt.Bind(1, new_room_name);
        stmt.Bind(2, current_room_name);
        bool success = sqlite3_step(stmt.Get()) == SQLITE_DONE;
        return success;
    }
        
    std::optional<User> DB::GetUserData(const std::string& user_login) {
        Stmt stmt(db_, sql::GET_USER_DATA);
        stmt.Bind(1, user_login);
        int rc = sqlite3_step(stmt.Get());

        if (rc == SQLITE_ROW) {
            std::string login = stmt.GetColumnText(0);
            std::string name = stmt.GetColumnText(1);
            std::string password_hash = stmt.GetColumnText(2);
            std::string role = stmt.GetColumnText(3);
            bool is_deleted = sqlite3_column_int(stmt.Get(), 4) != 0;
            int64_t unixtime = sqlite3_column_int64(stmt.Get(), 5);
            return User{ login, name, password_hash, role, is_deleted, unixtime };
        }
        std::cerr << "[GetUserData] SQL error or unexpected result (" << rc << "): "
            << sqlite3_errmsg(db_) << "\n";
        return std::nullopt;
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

    std::unordered_map<std::string, std::unordered_set<std::string>> DB::GetAllRoomWithRegisteredUsers() {
        std::unordered_map<std::string, std::unordered_set<std::string>> list_room_and_user;

        Stmt stmt(db_, sql::GET_ALL_PAIR_ROOMS_AND_USERS);
        int rc;
        while ((rc = sqlite3_step(stmt.Get())) == SQLITE_ROW) {
            list_room_and_user[stmt.GetColumnText(0)].insert(stmt.GetColumnText(1));
        }
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL error during fetch: " << sqlite3_errmsg(db_) << "\n";
        }
        return list_room_and_user;
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

