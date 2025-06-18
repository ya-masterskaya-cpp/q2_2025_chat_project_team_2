#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

namespace db {
    struct User {

        User(std::string login, std::string name, std::string password_hash, std::string role, bool is_deleted, int64_t unixtime) :
            login(login), name(name), password_hash(password_hash), role(role), is_deleted(is_deleted), unixtime(unixtime) {}

        std::string login;
        std::string name;
        std::string password_hash;
        std::string role;
        bool is_deleted;
        int64_t unixtime; //наносекунды
    };

    struct Message {
        
        Message(std::string message, int64_t unixtime, std::string user_login, std::string room):
            message(message), unixtime(unixtime), user_login(user_login), room(room){}
        
        std::string message;
        int64_t unixtime; //наносекунды
        std::string user_login;
        std::string room;
    };

    class DB {
    public:
        DB();
        explicit DB(const std::string& db_file);
        ~DB();

        // --- System ---
        bool OpenDB();
        void CloseDB();
        std::string GetVersionDB();

        // --- Users ---
        bool CreateUser(const User& user);
        // если числится хоть в одной комнате, удаления не будет, только пометка is_deleted = 1, т.н. мягкое  удаление:
        bool DeleteUser(const std::string& user_login);
        bool IsUser(const std::string& user_login);
        bool IsAliveUser(const std::string& user_login);
        std::vector<User> GetAllUsers();
        std::vector<User> GetActiveUsers();
        std::vector<User> GetDeletedUsers();
        std::vector<std::string> GetUserRooms(const std::string& user_login);

        // --- Rooms ---
        bool CreateRoom(const std::string& room, int64_t unixtime);
        // при удалении комнаты в БД происходит автоматическое удаление из TABLE messages сообщений удаляемой комнаты:
        bool DeleteRoom(const std::string& room);
        bool IsRoom(const std::string& room);
        bool AddUserToRoom(const std::string& user_login, const std::string& room);
        std::vector<User> GetRoomActiveUsers(const std::string& room);
        // без удаления сообщений пользователя в комнате, только из TABLE user_rooms:
        bool DeleteUserFromRoom(const std::string& user_login, const std::string& room);
        std::vector<std::string> GetRooms();

        // --- Messages ---
        bool InsertMessageToDB(const Message& message); //
        // первые 50 сообщений (последние по времени создания):
        std::vector<Message> GetRecentMessagesRoom(const std::string& room);
        // следующие 50 после временной метки unixtime:
        std::vector<Message> GetMessagesRoomAfter(const std::string& room, int64_t unixtime);
        int GetCountRoomMessages(const std::string& room);

    private:
        sqlite3* db_ = nullptr;
        std::string db_filename_ = "chat.db";

        bool InitSchema();
        bool SetUserForDelete(const std::string& user_login);
        bool PerformSQLReturnBool(const char* sql_query, std::vector<std::string> param);
        // окончательное  удаление пользователей помеченных как удаленные, если нет комнат с пользователем, удаляем его из БД
        bool DelDeletedUsersWithoutRoom();
        std::vector<User> GetUsers(const char* sql);
    };
} // db

