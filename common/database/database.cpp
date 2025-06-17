#include "database.h"
#include <stdexcept>
#include <iostream>

Database::Database(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK){
        throw std::runtime_error("Cannot open database");
    }

    //создаем таблицу, если ее еще нет
    Execute(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "content TEXT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
    );
}

Database::~Database() {
    sqlite3_close(db);
}

bool Database::Execute(const std::string& sql) {
    char* msg_error = nullptr;
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &msg_error) == SQLITE_OK;
}

bool Database::InsertMessage(const std::string& message) {
    std::string sql = "INSERT INTO messages (content) VALUES ('" + message + "')";;
    return Execute(sql);
}

std::vector<std::string> Database::GetAllMessages() {
    std::vector<std::string> messages;
    sqlite3_stmt* stmt;

    const char* sql = "SELECT content FROM messages ORDER BY timestamp";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            messages.push_back(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))
            );
        }
        sqlite3_finalize(stmt);
    }
    return messages;
}


bool Database::ClearMessages() {
    return Execute("DELETE FROM messages;");
}