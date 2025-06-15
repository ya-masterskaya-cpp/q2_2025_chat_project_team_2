#pragma once
#include <sdkddkver.h>
#define BOOST_DISABLE_CURRENT_LOCATION

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <vector>
#include "json.h"
#include <mutex>
#include <condition_variable>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

const std::string def_host = "127.0.0.1";
const std::string def_in_port = "9003";
const std::string def_out_port = "9002";

const int GENERAL = 101;
const int LOGIN = 111;
const int CHANGE_NAME = 112;
const int CREATE_ROOM = 113;
const int ENTER_ROOM = 114;
const int ASK_ROOMS = 115;

struct MesQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<std::string> msg_;
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
        while (true) {
            cv_.wait(lk, [&] { return msg_.size() > 0; } );
            for (auto& mes : msg_) {
                func(mes);
            }
            msg_.clear();
        }
    }
};

struct MesBuilder
{
    MesBuilder(int type) {
        j["type"] = type;
    }
    MesBuilder& add(const std::string key, const std::string value) {
        j[key] = value;
        return *this;
    }
    MesBuilder& add(const std::string& key, const std::vector<std::string>& value) {
        j[key] = value;
        return *this;
    }
    std::string toString() {
        return j.dump();
    }
    nlohmann::json j;
};

using MessageHandler = std::function<void(const std::string&)>;
class Client
{
public:
    void start(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        std::thread(&Client::sender, this, host, in_port, out_port).detach();
    }

    void set_handler(MessageHandler mh) {
        message_handler = mh;
    }

    void send_message(const std::string& room, const std::string& message) {
        mq_.add(
            MesBuilder(GENERAL).add("room", room).add("content", message).toString()
        );
    }

    void register_user(const std::string& user, const std::string& password) {
        mq_.add(
            MesBuilder(LOGIN).add("user", user).add("password", password).toString()
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

private:
    void sender(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        websocket::stream<tcp::socket> ws(ioc);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, out_port);
        try {
            asio::connect(ws.next_layer(), results);
            ws.handshake(host, "/");
            std::thread(&Client::getter, this, host, in_port).detach();
            mq_.wait_on_message([&](const std::string& s) {
                ws.write(asio::buffer(s));
                });           
        }
        catch (...) {
            message_handler("Error connect");
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
                ws.read(buffer);
                message_handler(beast::buffers_to_string(buffer.data()));
                buffer.consume(buffer.size());
            }
        }
        catch (...) {
            message_handler("Read or connection error");
        }
    }
    asio::io_context ioc;
    MesQueue mq_;
    bool finish_ = false;
    MessageHandler message_handler;
};

