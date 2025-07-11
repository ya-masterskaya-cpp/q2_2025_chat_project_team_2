// test/server_tests.cpp

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

#include "server.h"  
#include "db.hpp"
#include "time_utils.hpp"
#include "../../../src/client/ClientHTTP.h"
#include "../../common/chat_server_lib/src/json.h"
#include "Logger.h"
/*

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
    TestableServer() : Server() {}

    ~TestableServer() {
        StopForTest();
    }

    void run_server(db::DB& db) override {
        logger_.logEvent("TestableServer starting...");

        db_ = &db;

        // загружаем пользователей из БД
        auto users = db_->GetAllUsers();
        for (const auto& user : users) {
            users_passwords_[user.login] = user.password_hash;
        }

        test_is_running_ = true;
        // Запускаем потоки
        test_threads_.emplace_back(&TestableServer::shuttle, this);
        test_threads_.emplace_back(&TestableServer::client_accept, this);

        logger_.logEvent("TestableServer initialized and running in background.");
    }

    void StopForTest() {
        // Убедимся, что останавливаем только один раз
        if (!test_is_running_.exchange(false)) {
            return;
        }

        // 1. Останавливаем io_context ПЕРВЫМ. Это разблокирует ВСЕ операции asio
        //    и заставит потоки getter, sender и client_accept выйти из блокирующих вызовов.
        ioc.stop();

        // 2. Будим шаттл, чтобы он проверил флаг и завершился.
        in_msg.invalidate();

        // 3. Ждем завершения основных потоков
        for (auto& t : test_threads_) {
            if (t.joinable()) t.join();
        }

        // 4. Ждем завершения потоков клиентов
        std::lock_guard<std::mutex> lock(client_threads_mutex_);
        for (auto& t : test_client_threads_) {
            if (t.joinable()) t.join();
        }
        test_client_threads_.clear(); // Очищаем вектор после

        logger_.logEvent("TestableServer stopped.");
    }

    // В классе TestableServer
    void client_accept() override {
        try {
            // Инициализируем акцепторы один раз при запуске потока.
            // Используем опцию для переиспользования адреса, чтобы избежать проблем
            // при быстром перезапуске тестов.
            boost::asio::ip::tcp::resolver resolver(ioc);

            in_acceptor_.open(tcp::v4());
            in_acceptor_.set_option(tcp::acceptor::reuse_address(true));
            in_acceptor_.bind(tcp::endpoint(tcp::v4(), in_port));
            in_acceptor_.listen();

            out_acceptor_.open(tcp::v4());
            out_acceptor_.set_option(tcp::acceptor::reuse_address(true));
            out_acceptor_.bind(tcp::endpoint(tcp::v4(), out_port));
            out_acceptor_.listen();

        }
        catch (const std::exception& e) {
            logger_.logError("Test acceptor failed to bind: " + std::string(e.what()));
            // Если не удалось открыть порты, поток завершается.
            return;
        }

        logger_.logEvent("Test acceptor is running...");

        // Цикл управляется тестовым флагом
        while (test_is_running_) {
            try {
                tcp::socket in_socket(ioc);
                // Используем accept() на члене класса
                in_acceptor_.accept(in_socket);

                // Проверка после разблокировки, на случай если нас остановили, пока мы ждали
                if (!test_is_running_) break;

                tcp::socket out_socket(ioc);
                out_acceptor_.accept(out_socket);

                MsgQueue* session = new MsgQueue;

                // Блокируем мьютекс для безопасного доступа к вектору потоков клиентов
                std::lock_guard<std::mutex> lock(client_threads_mutex_);
                // Создаем и сохраняем потоки клиента, НЕ используя detach()
                test_client_threads_.emplace_back(&TestableServer::getter, this, std::move(in_socket), session);
                test_client_threads_.emplace_back(&TestableServer::sender, this, std::move(out_socket), session);
            }
            catch (const beast::system_error& e) {
                // Эта ошибка возникнет, когда мы вызовем ioc.stop() или acceptor.close().
                // Это нормальный способ завершения потока.
                if (!test_is_running_ || e.code() == asio::error::operation_aborted) {
                    logger_.logEvent("Test acceptor loop terminated as expected.");
                    break; // Выходим из цикла
                }
                // Логируем другие, неожиданные сетевые ошибки
                logger_.logError("Network error in test acceptor: " + e.code().message());
            }
            catch (const std::exception& e) {
                // Логируем любые другие неожиданные ошибки
                logger_.logError("Generic error in test acceptor: " + std::string(e.what()));
            }
        }
        logger_.logEvent("Test client_accept thread finished.");
    }

    // В классе TestableServer
    void shuttle() override {
        logger_.logEvent("Test shuttle is running...");

        // wait_on_queue в MsgQueue ждет, пока is_online не станет false.
        // Наш метод StopForTest вызовет in_msg.invalidate(), который это и сделает.
        in_msg.wait_on_queue([this](const nlohmann::json& j) {
            // Добавляем дополнительную проверку флага внутри лямбды для надежности.
            // Это предотвратит обработку сообщений, если остановка уже началась.
            if (!test_is_running_) {
                return;
            }

            // Проверяем, что сообщение корректно и комната существует
            if (j.contains("room")) {
                auto room_name = j["room"].get<std::string>();
                auto it = users_.find(room_name);
                if (it != users_.end()) {
                    // Итерируемся по сессиям в комнате
                    for (auto ses : it->second) {
                        if (ses) { // Простая проверка, что указатель не нулевой
                            ses->add(j);
                        }
                    }
                }
            }
            });

        logger_.logEvent("Test shuttle thread finished.");
    }

    std::vector<std::thread> test_client_threads_; // <-- НОВЫЙ ВЕКТОР для потоков клиентов
    std::mutex client_threads_mutex_; // <-- Мьютекс для защиты этого вектора

    std::vector<std::thread> test_threads_;
    std::atomic<bool> test_is_running_{ false };

    using Server::add_user;
    using Server::ask_rooms;
    using Server::ask_users;
    using Server::change_name;
    using Server::change_session;
    using Server::check_login;
    using Server::create_room;
    using Server::enter_room;
    using Server::getter;
    using Server::leave_room;
    using Server::make_err_answer;
    using Server::make_ok_answer;
    using Server::register_user;
    using Server::remove_user;
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

    // Спарсим оба JSON, чтобы сравнение не зависело от порядка ключей
    auto j_actual = nlohmann::json::parse(builder.toString());
    auto j_expected = nlohmann::json::parse(expected);

    REQUIRE(j_actual == j_expected);
}

TEST_CASE("Server and BD with users") {
    TestDatabase db_tst;
    auto& db = db_tst.database;
    db::User user{ "user1", "Name", "hash", "user", false, utime::GetUnixTimeNs() };
    db.CreateUser(user);


    SECTION("check_login") {
        TestableServer server;
        server.run_server(db);
        REQUIRE(server.check_login("user1", "hash") == true);
        REQUIRE(server.check_login("user2", "hash") == false);
        REQUIRE(server.check_login("user1", "hash1") == false);
        server.StopForTest();
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