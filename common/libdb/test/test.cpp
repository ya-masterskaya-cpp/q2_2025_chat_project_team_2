#define CATCH_CONFIG_MAIN  // Должен быть только в одном файле
#include <catch2/catch_test_macros.hpp>
#include "db.hpp"
#include "time_utils.hpp"


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

   SECTION("Change user name") {
       db.CreateUser(test_user);
       auto users = db.GetActiveUsers();
       REQUIRE(users[0].name == "Test User");
       REQUIRE(db.ChangeUserName("test_login", "New Name User") == true);
       auto new_users = db.GetActiveUsers();
       REQUIRE(new_users[0].name == "New Name User");
   }

   SECTION("Get user info") {
       db.CreateUser(test_user);
       auto user_info = db.GetUserData("test_login");
       REQUIRE(user_info->name == "Test User");
       auto user_info_not_exist = db.GetUserData("test");  // здесь выводится ошибка [GetUserData] SQL error or unexpected result (101): no more rows available, это правильно
       REQUIRE(user_info_not_exist == std::nullopt);
   }

}

TEST_CASE("Room management 1") {
    db::DB db(":memory:");
    db.OpenDB();

    SECTION("Create room") {
        REQUIRE(db.CreateRoom("room", utime::GetUnixTimeNs()) == true);
        REQUIRE(db.IsRoom("room") == true);
    }

    SECTION("Delete room") {
        db.CreateRoom("room", utime::GetUnixTimeNs());
        REQUIRE(db.IsRoom("room") == true);
        REQUIRE(db.DeleteRoom("room") == true);
        REQUIRE(db.IsRoom("room") == false);
    }

    SECTION("Change Room Name") {
        db.CreateRoom("room", utime::GetUnixTimeNs());
        REQUIRE(db.IsRoom("room") == true);
        REQUIRE(db.ChangeRoomName("room", "new_room") == true);
        REQUIRE(db.IsRoom("new_room") == true);
        REQUIRE(db.IsRoom("room") == false);
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

    SECTION("Add user to room and verify all user fields") {
        REQUIRE(db.AddUserToRoom("user1", "general") == true);

        auto room_users = db.GetRoomActiveUsers("general");
        REQUIRE(room_users.size() == 1);

        const auto& fetched_user = room_users[0];
        REQUIRE(fetched_user.login == "user1");
        REQUIRE(fetched_user.name == "Name");         
        REQUIRE(fetched_user.password_hash == "hash");
        REQUIRE(fetched_user.role == "user");       
        REQUIRE(fetched_user.is_deleted == false);  
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
TEST_CASE("Duplicate entries handling") {
    db::DB db(":memory:");
    db.OpenDB();

    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db.CreateUser(user);
    db.CreateRoom("general", utime::GetUnixTimeNs());

    SECTION("Creating a user with an existing login should be ignored") {
        db::User duplicate_user{ "user1", "Another Name", "another_hash", "admin", false, utime::GetUnixTimeNs() };
        REQUIRE(db.CreateUser(duplicate_user) == true); // INSERT OR IGNORE вернет success
        REQUIRE(db.GetAllUsers().size() == 1);
        auto users = db.GetAllUsers();
        REQUIRE(users[0].name == "Name"); // Проверяем, что данные не изменились
    }

    SECTION("Adding a user to the same room twice should be ignored") {
        REQUIRE(db.AddUserToRoom("user1", "general") == true);
        REQUIRE(db.AddUserToRoom("user1", "general") == true); // INSERT OR IGNORE
        REQUIRE(db.GetUserRooms("user1").size() == 1);
    }
}
TEST_CASE("Operations with non-existent entities") {
    db::DB db(":memory:");
    db.OpenDB();
    db.CreateRoom("general", utime::GetUnixTimeNs());
    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db.CreateUser(user);

    SECTION("Adding user to a non-existent room") {
        // Подзапрос в ADD_USER_TO_ROOM вернет NULL, INSERT не произойдет.
        // sqlite3_step вернет DONE, но изменений не будет.
        REQUIRE(db.AddUserToRoom("user1", "non_existent_room") == true);
        REQUIRE(db.GetUserRooms("user1").empty());
    }

    SECTION("Getting messages from an empty or non-existent room") {
        REQUIRE(db.GetRecentMessagesRoom("non_existent_room").empty());
        REQUIRE(db.GetCountRoomMessages("non_existent_room") == 0); 
    }
}
