#define CATCH_CONFIG_MAIN  // Должен быть только в одном файле
#include <catch2/catch_test_macros.hpp>
#include "../include/db.hpp"
#include "../src/time_utils.hpp"

TEST_CASE("DB initialization") {
    db::DB db(":memory:");
    REQUIRE(db.OpenDB() == true);
    REQUIRE(db.GetVersionDB().empty() == false);
}
TEST_CASE("User management 1") {
    db::DB db(":memory:");
    db.OpenDB();

    db::User test_user{
        "test_login", "Test User",
        "hash", "user", false, utime::GetUnixTimeNs()
    };

    SECTION("Create user") {
        REQUIRE(db.CreateUser(test_user) == true);
        REQUIRE(db.IsUser("test_login") == true);
    }

    SECTION("Delete user. User don`t have messages in rooms") {
        db.CreateUser(test_user);
        REQUIRE(db.GetAllUsers().size() == 1);
        REQUIRE(db.DeleteUser("test_login") == true);
        REQUIRE(db.IsUser("test_login") == false);
        REQUIRE(db.GetAllUsers().empty() == true);
    }
}
TEST_CASE("Room management 1") {
    db::DB db(":memory:");
    db.OpenDB();

    SECTION("Create room") {
        REQUIRE(db.CreateRoom("general", utime::GetUnixTimeNs()) == true);
        REQUIRE(db.IsRoom("general") == true);
    }

    SECTION("Delete room") {
        REQUIRE(db.DeleteRoom("general") == true);
        REQUIRE(db.IsRoom("general") == false);
    }
}
TEST_CASE("User and room management") {
    db::DB db(":memory:");
    db.OpenDB();
    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db.CreateUser(user);
    db.CreateRoom("general", utime::GetUnixTimeNs());
    
    SECTION("Add user to room") {
        REQUIRE(db.AddUserToRoom("user1", "general") == true);
        REQUIRE(db.GetUserRooms("user1")[0] == "general");
        REQUIRE(db.GetRoomActiveUsers("general")[0].login == "user1");
    }

    SECTION("Delete user from room") {
        db.AddUserToRoom("user1", "general");
        REQUIRE(db.DeleteUserFromRoom("user1", "general") == true);
        REQUIRE(db.IsAliveUser("user1") == true);
    }
}
TEST_CASE("Message management"){

    db::DB db(":memory:");
    db.OpenDB();

    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db::Message msg{ "Hello", utime::GetUnixTimeNs(), "user1", "general" };

    db.CreateRoom("general", utime::GetUnixTimeNs());
    db.CreateUser(user);
    db.AddUserToRoom("user1", "general");

    SECTION("Write message to DB") {
        REQUIRE(db.GetCountRoomMessages("general") == 0);
        REQUIRE(db.InsertMessageToDB(msg) == true);
        REQUIRE(db.GetCountRoomMessages("general") == 1);
    }
}
TEST_CASE("Room management 2"){

    db::DB db(":memory:");
    db.OpenDB();

    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db::Message msg{ "Hello", utime::GetUnixTimeNs(), "user1", "general" };

    db.CreateRoom("general", utime::GetUnixTimeNs());
    db.CreateUser(user);
    db.AddUserToRoom("user1", "general");
    db.InsertMessageToDB(msg);

    // при удалении комнаты происходит удаление сообщений из БД, привязки к пользователю
    SECTION("Delete room and messages room in table messages") {
        REQUIRE(db.GetCountRoomMessages("general") == 1);
        REQUIRE(db.IsRoom("general") == true);
        REQUIRE(db.GetRooms()[0] == "general");
        REQUIRE(db.GetUserRooms("user1")[0] == "general");

        REQUIRE(db.DeleteRoom("general") == true);

        REQUIRE(db.GetCountRoomMessages("general") == 0);
        REQUIRE(db.IsRoom("general") == false);
        REQUIRE(db.GetRooms().empty() == true);
        REQUIRE(db.GetUserRooms("user1").empty() == true);
    }
}
TEST_CASE("Message History and Pagination") {

    db::DB db(":memory:");
    db.OpenDB();

    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs()};
    db::Message msg{ "Hello", utime::GetUnixTimeNs(), "user1", "general" };
    
    db.CreateRoom("general", utime::GetUnixTimeNs());
    db.CreateUser(user);
    db.AddUserToRoom("user1", "general");

    SECTION("Recent messages") {
       REQUIRE(db.InsertMessageToDB(msg));
       REQUIRE(db.GetCountRoomMessages("general") == 1);
 
       auto messages = db.GetRecentMessagesRoom("general");
       REQUIRE(messages.size() == 1);
       REQUIRE(messages[0].message == "Hello");
    }

    SECTION("Pagination and get count messages") {
        // Добавляем 60 сообщений
        for (int i = 0; i < 60; i++) {
            int64_t u_time = utime::GetUnixTimeNs();
            db.InsertMessageToDB({ std::to_string(i), u_time, "user1", "general" });
        }
        REQUIRE(db.GetCountRoomMessages("general") == 60);

        auto first_page = db.GetRecentMessagesRoom("general");
        REQUIRE(first_page.size() == 50);

        auto second_page = db.GetMessagesRoomAfter("general", first_page.back().unixtime);
        REQUIRE(second_page.size() == 10);
    }
}
TEST_CASE("User management 2") {

    db::DB db(":memory:");
    db.OpenDB();

    int64_t utime_user = utime::GetUnixTimeNs();
    db::User user{ "user1", "Name", "hash", "user", false, utime_user };
    
    int64_t utime_message = utime::GetUnixTimeNs();
    db::Message msg{ "Hello", utime_message, "user1", "general" };

    int64_t utime_room = utime::GetUnixTimeNs();
    db.CreateRoom("general", utime_room);
    db.CreateUser(user);
    db.AddUserToRoom("user1", "general");
    db.InsertMessageToDB(msg);

    SECTION("write data == read data") {
        auto users = db.GetAllUsers();
        REQUIRE(users[0].login == "user1");
        REQUIRE(users[0].name == "Name");
        REQUIRE(users[0].password_hash == "hash");
        REQUIRE(users[0].role == "user");
        REQUIRE(users[0].unixtime == utime_user);

        REQUIRE(db.GetUserRooms("user1")[0] == "general");

        auto messages = db.GetRecentMessagesRoom("general");
        REQUIRE(messages[0].message == "Hello");
        REQUIRE(messages[0].room == "general");
        REQUIRE(messages[0].unixtime == utime_message);
        REQUIRE(messages[0].user_login == "user1");
    }

    SECTION("Soft and hard delete user") {

        REQUIRE(db.GetAllUsers().size() == 1);
        REQUIRE(db.GetActiveUsers().size() == 1);
        REQUIRE(db.GetDeletedUsers().size() == 0);
        
        db.DeleteUser("user1");

        REQUIRE(db.GetAllUsers().size() == 1);
        REQUIRE(db.GetActiveUsers().size() == 0);
        REQUIRE(db.GetDeletedUsers().size() == 1);

        db.DeleteRoom("general");

        REQUIRE(db.GetAllUsers().size() == 0);
        REQUIRE(db.GetActiveUsers().size() == 0);
        REQUIRE(db.GetDeletedUsers().size() == 0);
    }
}