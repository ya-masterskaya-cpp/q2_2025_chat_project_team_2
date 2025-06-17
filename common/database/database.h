#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    Database(const std::string& db_name);
    ~Database();

    bool Execute(const std::string& sql);
    bool InsertMessage(const std::string& message);
    std::vector<std::string> GetAllMessages();
    bool ClearMessages();

private:
    sqlite3* db;
};