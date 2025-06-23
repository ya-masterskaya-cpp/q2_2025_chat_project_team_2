// test/server_tests.cpp

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>
/*
#include "../../../src/server/server.h"  
#include "db.hpp"
#include "../src/time_utils.hpp"
#include "../../../src/server/ClientHTTP.h"
#include "../../../src/server/json.h"
#include "../../../src/server/Logger.h"


struct TestDatabase {
    db::DB database;
    const std::string db_path = "test.db";

    TestDatabase() {
        std::filesystem::remove(db_path);
        database = db::DB(db_path);
        REQUIRE(database.OpenDB() == true);
    }

    ~TestDatabase() {
        database.CloseDB();
        std::filesystem::remove(db_path);
    }
};

// Класс-наследник для тестирования, который "раскрывает"
// protected члены базового класса Server. 
class TestableServer : public Server {
public:

    void run_server(db::DB& db) override {
        logger_.logEvent("TestableServer starting...");

        // Открываем БД 
        db_ = &db;

        // загружаем пользователей из БД
        auto users = db_->GetActiveUsers();
        for (const auto& user : users) {
            users_passwords_[user.login] = user.password_hash;
        }

        // Запускаем потоки
        std::thread(&TestableServer::shuttle, this).detach();
        std::thread(&TestableServer::client_accept, this).detach();

        logger_.logEvent("TestableServer initialized and running in background.");
    }

    using Server::add_user;
    using Server::ask_rooms;
    using Server::change_name;
    using Server::change_session;
    using Server::check_login;
    using Server::client_accept;
    using Server::create_room;
    using Server::enter_room;
    using Server::getter;
    using Server::leave_room;
    using Server::login_user;
    using Server::make_err_answer;
    using Server::make_ok_answer;
    using Server::register_user;
    using Server::remove_user;
    using Server::run_server;
    using Server::shuttle;
    using Server::sender;

    using Server::logger_;
    using Server::in_msg;
    using Server::ioc;
    using Server::logged_users_;
    using Server::users_;
    using Server::users_passwords_;
    using Server::db_;

};

TEST_CASE("Server class can be instantiated", "[server]") {
    // Проверка, что код сервера в принципе собирается
    REQUIRE_NOTHROW(TestableServer{});
}

// Пример теста для структуры MesBuilder
TEST_CASE("MesBuilder creates correct JSON", "[server][protocol]") {
    MesBuilder builder(101); // GENERAL
    builder.add("room", "test_room").add("content", "hello");

    std::string expected = R"({"content":"hello","room":"test_room","type":101})";

    std::cout << "test server" << std::endl;

    // Спарсим оба JSON, чтобы сравнение не зависило от порядка ключей
    auto j_actual = nlohmann::json::parse(builder.toString());
    auto j_expected = nlohmann::json::parse(expected);

    REQUIRE(j_actual == j_expected);
}
*/

/*

TEST_CASE("Server and BD with users") {
    TestDatabase db_tst;
    auto& db = db_tst.database;
    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db.CreateUser(user);
    db.CloseDB();

    SECTION("Check user") {
        TestableServer server;
        server.run_server();
        MsgQueue session;
        REQUIRE(server.check_login(&session, "user1", "hash") == true);
        REQUIRE(server.check_login(&session, "user2", "hash") == false);
        REQUIRE(server.check_login(&session, "user1", "hash1") == false);
    }
}
/*
    SECTION("Create user") {
        TestableServer server;
        server.run_server();
        MsgQueue session;
        server.register_user(&session, "login", "password");
        //REQUIRE(server.db_->IsUser(login) == true);
        //REQUIRE(server.check_login(&session, "user1", "hash1") == false);
    }
}
*/