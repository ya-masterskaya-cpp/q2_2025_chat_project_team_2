#pragma once
#include <sdkddkver.h>
#define BOOST_DISABLE_CURRENT_LOCATION

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <vector>
//#include "json.h"
#include "nlohmann/json.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include "General.h"
#include "common_struct.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

const std::string def_host = "127.0.0.1";
const std::string def_in_port = "9003";
const std::string def_out_port = "9002";

struct MesQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<std::string> msg_;
    bool& finish_ref_; 

    MesQueue(bool& finish_flag) : finish_ref_(finish_flag) {}

    void add(const std::string& s) {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_.push_back(s);
        }
        cv_.notify_all();
    }

    template<typename F>
    void wait_on_message(F func) {
        std::unique_lock<std::mutex> lk(m_);
        while (!finish_ref_) {
            cv_.wait(lk, [&] { return !msg_.empty() || finish_ref_; } );

            if (finish_ref_ && msg_.empty()) {
                break;
            }

            for (auto& mes : msg_) {
                func(mes);
            }
            msg_.clear();      
        }
    }
};

using MessageHandler = std::function<void(const std::string&)>;
class Client
{
public:
    Client() : mq_(finish_) {}

    void start(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        start_thread_ = std::thread(&Client::sender, this, host, in_port, out_port);
    }

    void set_handler(MessageHandler mh) {
        message_handler = mh;
    }

    void send_message(const std::string& room, const std::string& message) {
        mq_.add(
            MesBuilder(GENERAL).add("room", room).add("content", message).toString()
        );
    }

    void login_user(const std::string& user, const std::string& password) {
        mq_.add(
            MesBuilder(LOGIN).add("user", user).add("password", password).toString()
        );
    }

    void register_user(const std::string& user, const std::string& password) {
        mq_.add(
            MesBuilder(REGISTER).add("user", user).add("password", password).toString()
        );
    }

    void check_login(const std::string& user, const std::string& password) {
        mq_.add(
            MesBuilder(CHECK_LOGIN).add("user", user).add("password", password).toString()
        );
    }

    void change_name(const std::string& name) {
        mq_.add(
            MesBuilder(CHANGE_NAME).add("name", name).toString()
        );
    }

    void create_room(const std::string& room) {
        mq_.add(
            MesBuilder(CREATE_ROOM).add("room", room).toString()
        );
    }

    void enter_room(const std::string& room) {
        mq_.add(
            MesBuilder(ENTER_ROOM).add("room", room).toString()
        );
    }

    void ask_rooms() {
        mq_.add(
            MesBuilder(ASK_ROOMS).toString()
        );
    }

    void leave_room(const std::string& room) {
        mq_.add(
            MesBuilder(LEAVE_ROOM).add("room", room).toString()
        );
    }

    void leave_chat() {
        mq_.add(
            MesBuilder(LEAVE_CHAT).toString()
        );
    }

    void ask_users(const std::string& room) {
        mq_.add(
            MesBuilder(ASK_USERS).add("room", room).toString()
        );
    }  

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mq_.m_);
            finish_ = true;
        }
        mq_.cv_.notify_all();

        if (!ioc.stopped()) {
            ioc.stop();
        }
        
        if (start_thread_.joinable()) {
            start_thread_.join();
        }

        if (sender_thread_.joinable()) {
            sender_thread_.join();
        }
    }

    ~Client() {
        stop();
    }

private:
    void sender(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        websocket::stream<tcp::socket> ws(ioc);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, out_port);
        try {
            asio::connect(ws.next_layer(), results);
            ws.handshake(host, "/");
            sender_thread_ = std::thread(&Client::getter, this, host, in_port);
            mq_.wait_on_message([&](const std::string& s) {
                if (finish_) return; //обозначаем выход
                ws.write(asio::buffer(s));
                });           
        }
        catch (const std::exception& e) {
            if (!finish_) {
                //ловим ошибки когда не началось завершение работы
                std::cerr << "[SENDER_ERROR] " << e.what() << std::endl;
                message_handler("Error connect");
            }
        }
    }

    void getter(const std::string& host, const std::string& in_port) {
        websocket::stream<tcp::socket> ws(ioc);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, in_port);
        try {
            asio::connect(ws.next_layer(), results);
            ws.handshake(host, "/");
            std::string mesg_;
            beast::flat_buffer buffer;            
            while (true) {
                if (finish_) break; //проверка что не пора отключаться
                ws.read(buffer);
                message_handler(beast::buffers_to_string(buffer.data()));
                buffer.consume(buffer.size());
            }
        }
        catch (const std::exception& e) {
            if (!finish_) {
                //ловим ошибки когда не началось завершение работы
                std::cerr << "[GETTER_ERROR] " << e.what() << std::endl;
                message_handler("Read or connection error");
            }
            
        }
    }

    asio::io_context ioc;
    MesQueue mq_;
    bool finish_ = false; //флаг завершения работы, у тебя он был но не использовался
    MessageHandler message_handler;
    std::thread start_thread_;
    std::thread sender_thread_;

};

