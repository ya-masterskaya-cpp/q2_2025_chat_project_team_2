#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

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
    std::string message;
    int64_t unixtimes; //наносекунды
    std::string user_name;
    std::string room;
    std::string date;
    std::string time;
};

class DB {
public:
    DB();
    explicit DB(const std::string& db_file);
    ~DB();
    
    // --- System ---
    bool OpenDB();
    void CloseDB();
    //void ResetData();
    std::string GetVersionDB(); //

    // добавляет фейковые тестовые данные в БД: пользователей, комнаты, распределение пользователей по комнатам, сообщения в комнаты
    bool InsertTestDataToBD();    // ДЛЯ ТЕСТОВ, УДАЛИТЬ В РЕЛИЗЕ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool DeleteDBFile();          // ДЛЯ ТЕСТОВ, УДАЛИТЬ В РЕЛИЗЕ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool RecreateDB();            // ДЛЯ ТЕСТОВ, УДАЛИТЬ В РЕЛИЗЕ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // --- Users ---
    bool CreateUser(const User& user); 
    // нужно реализовать периодическую проверку и удаление пользователей с is_deleted = 1, если комнаты с ними удалились после удаления пользователя
    bool SetUserForDelete(const std::string& user_login);
    // если числится хоть в одной комнате, удаления не будет, только пометка is_deleted = 1, т.н. мягкое  удаление:
    bool DeleteUser(const std::string& user_login); 
    bool IsUser(const std::string& user_login);
    std::vector<User> GetUsers();
    // получение действующих пользователей (без пометки об удалении):
    int64_t GetCountUsers(); 
    std::vector<User> GetRoomUsers(const std::string& room);  
    //int64_t GetCountRoomUsers(const std::string& room);

    // --- Rooms ---
    bool CreateRoom(const std::string& room, int64_t unixtime);
    // при удалении комнаты в БД происходит автоматическое удаление из TABLE messages сообщений удаляемой комнаты:
    bool DeleteRoom(const std::string& room);  
    bool IsRoom(const std::string& room);
    bool AddUserToRoom(const std::string& user_login, const std::string& room);
    //bool IsUserAtRoom(const std::string& user_login, const std::string& room); 
    // без удаления сообщений пользователя в комнате, только из TABLE user_rooms:
    bool DeleteUserFromRoom(const std::string& user_login, const std::string& room); 
    std::vector<std::string> GetRooms();
    //int64_t GetCountRooms();
    std::vector<std::string> GetUserRooms(const std::string& room); 
    //int64_t GetCountUserRooms(const std::string& room);  //

    // --- Messages ---
    bool InsertMessageToRoom(const Message& message); //
    // первые 50 сообщений (последние по времени создания):
    std::vector<Message> GetRecentMessages(const std::string& room); 
    // следующие 50 после временной метки unixtimes:
    std::vector<Message> GetMessagesAfter(const std::string& room, int64_t unixtimes); 

private:
    sqlite3* db_ = nullptr;   
    std::string db_filename_ = "chat.db";

    bool InitSchema();
    std::vector<User> GetUsersBySQL(const char* sql_query);
    bool PerformSQLReturnBool(const char* sql_query, std::vector<std::string> param);
        // окончательное  удаление пользователей помеченных как удаленные, если нет комнат с пользователем, удаляем его из БД
    bool DelDeletedUsersWithoutRoom(); 
};
