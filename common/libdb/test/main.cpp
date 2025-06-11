#include <iostream>
#include "db.hpp"
#include <chrono>
#include <cstdint>

inline int64_t GetUnixTimeNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

void PrintUsers(const std::vector<User>& users) {
    for (const auto& user : users) {
        std::cout << "    login: " << user.login
            << ", name: " << user.name
            << ", password_hash: " << user.password_hash
            << ", role: " << user.role
            << ", unixtime: " << user.unixtime
            << "\n";
    }
}

int main() {

    DB db;
    db.OpenDB();
    db.RecreateDB();
    db.InsertTestDataToBD();

    auto rooms = db.GetRooms();
    std::cout << "rooms:" << "\n";
    for (const auto& room : rooms) {
        std::cout << "  " << room << "\n";
    }
    std::cout << "\n";

    std::cout << "users:" << "\n";
    auto users = db.GetUsers();
    PrintUsers(users);

    std::cout << "users in room:" << "\n";
    for (const auto& room : rooms) {
        std::cout << "  list of the users in " << room << ":\n";
        auto users_room = db.GetRoomUsers(room);
        PrintUsers(users_room);
    }



    
    /*
    std::cout << "DeleteRoom room_1 = " << db.DeleteRoom("room_1") << '\n';
    std::cout << "IsRoom room_1 = " << db.IsRoom("room_1") << '\n';
    std::cout << "CreateRoom room_1 = " << db.CreateRoom("room_1", GetUnixTimeNs()) << '\n';
    auto rooms = db.GetRooms();
    std::cout << "GetRooms = " << rooms[0] << '\n';
    std::cout << "IsRoom room_1 = " << db.IsRoom("room_1") << '\n';
    std::cout << "DeleteRoom room_1 = " << db.DeleteRoom("room_1") << '\n';
    std::cout << "IsRoom room_1 = " << db.IsRoom("room_1") << '\n';

    std::cout << '\n';

    std::cout << "IsUser new_user = " << db.IsUser("new_user") << '\n';
    std::cout << "CreateUser new_user = " << db.CreateUser({ "new_user", "Иван Иванов", "хеш_пароля", "user", 0, GetUnixTimeNs() }) << '\n';
    std::cout << "IsUser new_user = " << db.IsUser("new_user") << '\n';
    std::cout << "DeleteUser new_user = " << db.DeleteUser("new_user") << '\n';
    std::cout << "IsUser new_user = " << db.IsUser("new_user") << '\n';
*/
    db.CloseDB();

    return 0;
}